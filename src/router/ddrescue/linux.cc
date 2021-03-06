/*  GNU ddrescue - Data recovery tool
    Copyright (C) 2014 Antonio Diaz Diaz.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _FILE_OFFSET_BITS 64

#include <cstdio>
#include <algorithm>
#include <stdint.h>
#include <unistd.h>

#include "linux.h"

#ifdef USE_LINUX
#include <cctype>
#include <string>
#include <cstring>
#include <cerrno>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <linux/hdreg.h>


void sanitize_string( std::string & str )
  {
  for( unsigned i = str.size(); i > 0; --i )	// remove non-printable chars
    {
    const unsigned char ch = str[i-1];
    if( std::isspace( ch ) ) str[i-1] = ' ';
    else if( ch < 32 || ch > 126 ) str.erase( i - 1, 1 );
    }
  for( unsigned i = str.size(); i > 0; --i )	// remove duplicate spaces
    if( str[i-1] == ' ' && ( i <= 1 || i >= str.size() || str[i-2] == ' ' ) )
      str.erase( i - 1, 1 );
  }

const char * device_id( const int fd )
  {
  static std::string id_str;
  struct hd_driveid id;

  if( ioctl( fd, HDIO_GET_IDENTITY, &id ) != 0 ) return 0;
  id_str = (const char *)id.model;
  std::string id_serial( (const char *)id.serial_no );
  sanitize_string( id_str );
  sanitize_string( id_serial );
  if( id_str.size() || id_serial.size() )
    { id_str += "::"; id_str += id_serial; return id_str.c_str(); }
  return 0;
  }

void print_sgio_help( void )
  {
    fprintf(stderr, "      --scsi-passthrough         use ioctl sg_io direct SCSI passthrough-2.1\n"
                 "      --ata-passthrough          use ioctl sg_io direct ATA passthrough-2.1\n"
                 "      --mark-abnormal-error      mark abnormal error as non-trimmed-2.1\n" );
  }

void option_scsi_passthrough( void )
  {
    sgpt = true;
  }

void option_ata_passthrough( void )
  {
    sgpt = true;
    ata = 1;
  }

void option_mark_abnormal_error( void )
  {
    mark_error = true;
  }

int check_device( const char *iname, const int verbosity, const int cluster )
  {
    // extract last part of device name
    int i = 0;
    while (iname[i] != '\0')
      i++;
    while (iname[i] != '/')
    {
      i--;
      if (i < 0)
        break;
    }
    char device_name[20];
    i++;
    int n = 0;
    while (iname[i] != '\0')
    {
      device_name[n] = iname[i];
      n++;
      i++;
    }
    device_name[n] = '\0';

    // whether scsi or ata we need to get the buffer limit that the kernel uses
    // and we can also use this to find out if whole disk or partition
    char file_name[80] = "/sys/block/\0";
    strcat (file_name, device_name);
    strcat (file_name, "/queue/max_sectors_kb");

    FILE *file_pointer;
    file_pointer = fopen(file_name, "r");
    if (file_pointer == NULL)
    {
      if (verbosity > 4)
        std::fprintf(stderr, "Cannot open %s for reading (%s)\n", file_name, strerror(errno));
      std::fprintf(stderr, "Error! Input must be whole disk with passthrough option. Aborting...\n" );
      return 1;
    }
    else
    {
      char line[20];
      while (fgets(line, sizeof line, file_pointer))
        break;

      long long max_sectors_kb = strtoull(line, NULL, 0);
      if (cluster*sector_size > max_sectors_kb*1024)
      {
        std::fprintf(stderr, "Error! The reported cluster limit is %lld and you chose %d. Aborting...\n", (max_sectors_kb*1024)/sector_size, cluster);
        return 1;
      }
      fclose(file_pointer);
    }
    return 0;
  }






  // stuff for sgpt
  int buffer_size;
  bool ata_inquiry = false;
  unsigned char sense_buf[64];
  unsigned char scsi_cmd[16];
  unsigned char sense_key = 0;
  unsigned char asc = 0;
  unsigned char ascq = 0;
  int ioctl_ret;
  long long sector_start;
  long long sector_count;
  struct sg_io_hdr io_hdr;
  int command_length = 16;
  int prepare_cdb(uint8_t * const buf, const int verbosity);
  int post_ioctl(uint8_t * const buf, const int verbosity);
  int scsi_inquiry(const int fd, uint8_t * const buf, const int verbosity);



int read_linux( const int fd, uint8_t * const buf, const int size, const long long pos, int sz, const int verbosity )
{
  errno = 0;
  int n;
  // if passthough
  if (sgpt)
  {
    // check if either pos or size are dividable by the block size
    // if not then return with read error as it just can't be done properly
    // basically the same result as when using the --direct option
    if (pos % sector_size != 0 || size % sector_size != 0)
    {
      if (verbosity > 4)
        fprintf (stdout, "\nposition=%lld size=%d position or size not dividable\n", pos, size);
      errno = 1;
      return 0;
    }


    // perform initial inquiry
    if (!checked_ata)
      ata = scsi_inquiry(fd, buf, verbosity);

    if (ata < 0)
    {
      return (0);
    }

    sector_start = (pos / sector_size);
    sector_count = (size / sector_size);

    command_length = 16;
    buffer_size = size;
    bool keep_reading = true;
    int passes = 0;
    bad_ata_read = false;
    partial_read = false;
    passthrough_error = 0;

    while (keep_reading)
    {
      keep_reading = false;
      passes ++;
      // if disk is ata then do ata commands
      if (ata)
      {
        if (extended)
        {
          scsi_cmd[0] = 0x85; // ata 16 passthough
          scsi_cmd[1] = (0x06<<1)+1; // set protocol to 6 DMA, and set extended bit
          scsi_cmd[2] = (0<<5)+(1<<3)+(1<<2)+2; // unset check condition, direction (1 from device, 0 to device), block transfer, and length to sector field
          scsi_cmd[3] = 0;
          scsi_cmd[4] = 0;
          scsi_cmd[5] = (unsigned char)((sector_count >> 8) & 0xff);
          scsi_cmd[6] = (unsigned char)(sector_count & 0xff);
          scsi_cmd[7] = (unsigned char)((sector_start >> 24) & 0xff);
          scsi_cmd[8] = (unsigned char)(sector_start & 0xff);
          scsi_cmd[9] = (unsigned char)((sector_start >> 32) & 0xff);
          scsi_cmd[10] = (unsigned char)((sector_start >> 8) & 0xff);
          scsi_cmd[11] = (unsigned char)((sector_start >> 40) & 0xff);
          scsi_cmd[12] = (unsigned char)((sector_start >> 16) & 0xff);
          scsi_cmd[13] = (1<<7) + (1<<6) + (1<<5); // back-compat, set LBA bit, back-compat
          scsi_cmd[14] = 0x25; // read sectors DMA ext
          scsi_cmd[15] = 0;
        }

        else
        {
          scsi_cmd[0] = 0x85; // ata 16 passthough
          scsi_cmd[1] = (0x06<<1); // set protocol to 6 DMA
          scsi_cmd[2] = (0<<5)+(1<<3)+(1<<2)+2; // unset check condition, direction (1 from device, 0 to device), block transfer, and length to sector field
          scsi_cmd[3] = 0;
          scsi_cmd[4] = 0;
          scsi_cmd[5] = 0;
          scsi_cmd[6] = (unsigned char)(sector_count & 0xff);
          scsi_cmd[7] = 0;
          scsi_cmd[8] = (unsigned char)(sector_start & 0xff);
          scsi_cmd[9] = 0;
          scsi_cmd[10] = (unsigned char)((sector_start >> 8) & 0xff);
          scsi_cmd[11] = 0;
          scsi_cmd[12] = (unsigned char)((sector_start >> 16) & 0xff);
          scsi_cmd[13] = (1<<7) + (1<<6) + (1<<5) + (unsigned char)((sector_start >> 24) & 0xf);; // back-compat, set LBA bit, back-compat, high 4 bits of LBA
          scsi_cmd[14] = 0xC8; // read sectors DMA
          scsi_cmd[15] = 0;
        }
      }

      // otherwise do scsi commands
      else
      {
        // if the highest LBA to be read is bigger than 32 bits then use read 16
        if (sector_start * sector_count > 0xFFFFFFFF)
          command_length = 16;
        else
          command_length = 10;

        switch (command_length)
        {
          case 10:
            scsi_cmd[0] = 0x28; // read 10 command
            scsi_cmd[1] = 0;
            scsi_cmd[2] = (unsigned char)((sector_start >> 24) & 0xff);
            scsi_cmd[3] = (unsigned char)((sector_start >> 16) & 0xff);
            scsi_cmd[4] = (unsigned char)((sector_start>> 8) & 0xff);
            scsi_cmd[5] = (unsigned char)(sector_start & 0xff);
            scsi_cmd[6] = 0;
            scsi_cmd[7] = (unsigned char)((sector_count >> 8) & 0xff);
            scsi_cmd[8] = (unsigned char)(sector_count & 0xff);
            scsi_cmd[9] = 0;
            break;

          case 16:
            scsi_cmd[0] = 0x88; // read 16 command
            scsi_cmd[1] = 0;
            scsi_cmd[2] = (unsigned char)((sector_start >> 56) & 0xff);
            scsi_cmd[3] = (unsigned char)((sector_start >> 48) & 0xff);
            scsi_cmd[4] = (unsigned char)((sector_start >> 40) & 0xff);
            scsi_cmd[5] = (unsigned char)((sector_start >> 32) & 0xff);
            scsi_cmd[6] = (unsigned char)((sector_start >> 24) & 0xff);
            scsi_cmd[7] = (unsigned char)((sector_start >> 16) & 0xff);
            scsi_cmd[8] = (unsigned char)((sector_start >> 8) & 0xff);
            scsi_cmd[9] = (unsigned char)(sector_start & 0xff);
            scsi_cmd[10] = (unsigned char)((sector_count >> 24) & 0xff);
            scsi_cmd[11] = (unsigned char)((sector_count >> 16) & 0xff);
            scsi_cmd[12] = (unsigned char)((sector_count >> 8) & 0xff);
            scsi_cmd[13] = (unsigned char)(sector_count & 0xff);
            scsi_cmd[14] = 0;
            scsi_cmd[15] = 0;
            break;

          default:
            fprintf(stderr, "expected cdb size of 10 or 16 but got %d\n", command_length);
            fprintf (stderr, "\n\n\n\n");
            passthrough_error = 2;
            return (0);
        }
      }

      prepare_cdb(buf, verbosity);

      errno = 0;
      ioctl_ret = ioctl(fd, SG_IO, &io_hdr);
      if (ioctl_ret < 0)
      {
        perror("\nreading (SG_IO) on sg device, error");
        fprintf(stderr, "maybe you specified too large a cluster size for SG_IO to handle\n");
        fprintf (stderr, "\n\n\n\n");
        passthrough_error = 2;
        return (0);
      }

      post_ioctl(buf, verbosity);

      // if SG_INFO_OK_MASK is not 0 then something abnormal happened and we need to check
      if ((io_hdr.info & 0x01) != 0)
      {
        // if there is residual data left then set read error as it was not complete
        if (io_hdr.resid != 0)
        {
          errno = 1;
        }

        // if host status not 0 then something bad happened so set read error
        if (io_hdr.host_status != 0)
        {
          errno = 1;
        }

        // if driver status not 0 (good) or 8 (sense data written) then set read error;
        if (io_hdr.driver_status != 0 || io_hdr.driver_status != 8)
        {
          errno = 1;
        }
      }

      // if ata and there is an uncorectable error and read size is larger than one sector
      // then set bad sector and prepare to read the good data before the bad sector
      // this is skipped if there is an abnormal error
      if (!passthrough_error && ata && errno && (io_hdr.sbp[8+3] & 0x40) && size > sector_size)
      {
        keep_reading = true;
        long long bad_sector = 0;
        if (extended)
        {
          bad_sector += (long long)io_hdr.sbp[8+7];
          bad_sector += (long long)io_hdr.sbp[8+9] << 8;
          bad_sector += (long long)io_hdr.sbp[8+11] << 16;
          bad_sector += (long long)io_hdr.sbp[8+6] << 24;
          bad_sector += (long long)io_hdr.sbp[8+8] << 32;
          bad_sector += (long long)io_hdr.sbp[8+10] << 40;
        }
        else
        {
          bad_sector += (long long)io_hdr.sbp[8+7];
          bad_sector += (long long)io_hdr.sbp[8+9] << 8;
          bad_sector += (long long)io_hdr.sbp[8+11] << 16;
          bad_sector += ( ((long long)io_hdr.sbp[8+12]) & 0xf) << 24;
        }

        sector_count = bad_sector - sector_start;
        buffer_size = sector_count * sector_size;
        long long bad_offset = bad_sector * sector_size;

        if (verbosity > 4)
          fprintf(stdout, "first bad sector = %llX (offset=%llX)\n", bad_sector, bad_offset);
      }

      // if the first sector was bad then there is nothing to retry so return with 0 bytes read
      // we don't want to call the read with sector count of 0 as it will read the max
      if (sector_count < 1 && keep_reading)
      {
        errno = 1;
        return (0);
      }

      // if second pass and it still wants to read again return with 0 bytes read
      if (passes > 1 && keep_reading)
      {
        errno = 1;
        bad_ata_read = true;
        return (0);
      }

      // if failed on the first pass and no retry then return with 0 bytes read
      if (!keep_reading && passes == 1 && errno)
        return (0);

      // if second pass and good read then return with the data successfully read
        if (!keep_reading && passes > 1 && !errno)
        {
          errno = 1;
          partial_read = true;
          return (buffer_size);
        }

        // if we made it here on the second pass something went wrong
        // so return with 0 bytes read
        if (passes > 1)
        {
          errno = 1;
          bad_ata_read = true;
          return (0);
        }
    }
    n = size;
  }

  // if not passthough then normal read
  else
  {
    n = read( fd, buf + sz, size - sz );
  }

  return n;
}







  // function to perform initial inquiry
int scsi_inquiry(const int fd, uint8_t * const buf, const int verbosity)
  {
    bool doata = false;
    checked_ata = true;
    scsi_cmd[0] = 0x12; // inquiry command
    scsi_cmd[1] = 0;
    scsi_cmd[2] = 0;
    scsi_cmd[3] = 0;
    scsi_cmd[4] = 0x2C; // size of data requested back
    scsi_cmd[5] = 0;

    buffer_size = 256;
    command_length = 6;
    prepare_cdb(buf, verbosity);

    if (verbosity > 5)
      fprintf (stdout, "Performing SCSI Inquiry command\n");

    errno = 0;
    ioctl_ret = ioctl(fd, SG_IO, &io_hdr);
    if (ioctl_ret < 0)
    {
      perror("SCSI inquiry error");
      fprintf (stderr, "\n\n\n\n");
      passthrough_error = 2;
      return (-1);
    }

    post_ioctl(buf, verbosity);

    if (errno)
      return (0);

    if (strncmp((char *)buf+8, "ATA     ", 8) == 0)
    {
      if (ata)
        doata = true;
    }
    else
    {
      if (ata)
      {
        fprintf (stderr, "\nDevice is not ATA, Aborting...\n");
        fprintf (stderr, "\n\n\n\n");
        passthrough_error = 2;
        return (-1);
      }
    }


    // if device is ata then perform ata inquiry
    if (doata)
    {
      buffer_size = 512;
      command_length = 12;

      scsi_cmd[0] = 0xA1; // ata passthough 12 command
      scsi_cmd[1] = 0x08; // set protocol to 4, PIO Data-In
      scsi_cmd[2] = (1<<5)+(1<<3); // set check condition, direction (1 from device, 0 to device)
      scsi_cmd[3] = 0;
      scsi_cmd[4] = 0;
      scsi_cmd[5] = 0;
      scsi_cmd[6] = 0;
      scsi_cmd[7] = 0;
      scsi_cmd[8] = 0;
      scsi_cmd[9] = 0xEC; // identify device command
      scsi_cmd[10] = 0;
      scsi_cmd[11] = 0;

      prepare_cdb(buf, verbosity);

      if (verbosity > 5)
        fprintf (stdout, "Performing ATA Identify Device command\n");
      errno = 0;
      ioctl_ret = ioctl(fd, SG_IO, &io_hdr);
      if (ioctl_ret < 0)
      {
        perror("ATA inquiry error");
        fprintf (stderr, "\n\n\n\n");
        passthrough_error = 2;
        return (-1);
      }
      ata_inquiry = true;
      post_ioctl(buf, verbosity);
      ata_inquiry = false;

      // get extended bit from inquiry results
      // bit 10 of word 83, bytes are flipped in ata words
      extended = buf[167] & 4;
      if (verbosity > 5 && extended)
        fprintf (stdout, "Device supports 48 bit commands\n");
      if (verbosity > 5 && !extended)
        fprintf (stdout, "Device only supports 28 bit commands\n");
    }
    return doata;
  }





  // function to prepare CDB
int prepare_cdb(uint8_t * const buf, const int verbosity)
  {
    if (verbosity > 5)
    {
      int i;
      fprintf (stdout, "\nscsi_cmd= ");
      for (i = 0; i < command_length; i++)
      {
        fprintf (stdout, "%02X ", scsi_cmd[i]);
      }
      fprintf (stdout, "\n");

      if (ata)
      {
        fprintf(stdout, "ATA command data:\n");
        fprintf(stdout, "features= %02X%02X\n", scsi_cmd[3], scsi_cmd[4]);
        fprintf(stdout, "count= %02X%02X\n", scsi_cmd[5], scsi_cmd[6]);
        fprintf(stdout, "LBAhigh= %02X%02X\n", scsi_cmd[11], scsi_cmd[9]);
        fprintf(stdout, "LBAmid= %02X%02X\n", scsi_cmd[7], scsi_cmd[12]);
        fprintf(stdout, "LBAlow= %02X%02X\n", scsi_cmd[10], scsi_cmd[8]);
        fprintf(stdout, "device= %02X\n", scsi_cmd[13]);
        fprintf(stdout, "command= %02X\n", scsi_cmd[14]);
      }
    }

    memset(&io_hdr, 0, sizeof(struct sg_io_hdr));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = command_length;
    io_hdr.cmdp = scsi_cmd;
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = buffer_size;
    io_hdr.dxferp = buf;
    io_hdr.mx_sb_len = 64;
    io_hdr.iovec_count = 0;
    io_hdr.sbp = sense_buf;
    io_hdr.timeout = 30000; // timeout in ms, 30s should be enough for drive to recover from standby
    io_hdr.flags = SG_FLAG_DIRECT_IO;
    io_hdr.pack_id = 0;
    io_hdr.usr_ptr = 0;

    return (0);
  }





  // function post ioctl
int post_ioctl(uint8_t * const buf, const int verbosity)
  {
    // if there was sense data written then check for errors
    if (io_hdr.sb_len_wr != 0)
    {
      switch (io_hdr.sbp[0])
      {
        case 0x70:
        case 0x71:
          sense_key = io_hdr.sbp[2] & 0x0f;
          asc = io_hdr.sbp[12];
          ascq = io_hdr.sbp[13];
          break;

        case 0x72:
        case 0x73:
          sense_key = io_hdr.sbp[1] & 0x0f;
          asc = io_hdr.sbp[2];
          ascq = io_hdr.sbp[3];
          break;

        default:
          fprintf (stderr, "\nbad (SG_IO) sense buffer response code\n");
          fprintf (stderr, "\n\n\n\n");
          passthrough_error = 2;
          return (0);
      }

      if (sense_key != 0) // if not 0 then check further
      {
        if (sense_key != 1) // value of 1 means a recovered error or other soft error so read was still good, else read failed
              errno = 1;
      }

    }
    if (verbosity > 5 || (verbosity > 4 && errno))
    {
      fprintf(stdout, "status=%hhu, masked_status=%hhu, msg_status=%hhu, sb_len_wr=%hhu, host_status=%hu, driver_status=%hu, resid=%hhu, duration=%u, info=%u\n",
              io_hdr.status,
              io_hdr.masked_status,
              io_hdr.msg_status,
              io_hdr.sb_len_wr,
              io_hdr.host_status,
              io_hdr.driver_status,
              io_hdr.resid,
              io_hdr.duration,
              io_hdr.info);

      // if sense data then show it also
      if (io_hdr.sb_len_wr != 0)
      {
        fprintf(stdout, "additional sense info: %02X %02X %02X\n", sense_key, asc, ascq);
        fprintf(stdout, "sense buffer, in hex:\n");
        unsigned int i;
        for (i = 0; i < io_hdr.sb_len_wr; i++)
          fprintf(stdout, "%02X ", io_hdr.sbp[i]);
        fprintf(stdout, "\n");
        if (ata)
        {
          int ata_shift = 8;
          fprintf(stdout, "ATA return data:\n");
          fprintf(stdout, "descriptor= %02X\n", io_hdr.sbp[ata_shift+0]);
          fprintf(stdout, "additional length= %02X\n", io_hdr.sbp[ata_shift+1]);
          fprintf(stdout, "extend= %02X\n", io_hdr.sbp[ata_shift+2]);
          fprintf(stdout, "error= %02X\n", io_hdr.sbp[ata_shift+3]);
          fprintf(stdout, "count= %02X%02X\n", io_hdr.sbp[ata_shift+4], io_hdr.sbp[ata_shift+5]);
          fprintf(stdout, "LBAhigh= %02X%02X\n", io_hdr.sbp[ata_shift+10], io_hdr.sbp[ata_shift+8]);
          fprintf(stdout, "LBAmid= %02X%02X\n", io_hdr.sbp[ata_shift+6], io_hdr.sbp[ata_shift+11]);
          fprintf(stdout, "LBAlow= %02X%02X\n", io_hdr.sbp[ata_shift+9], io_hdr.sbp[ata_shift+7]);
          fprintf(stdout, "device= %02X\n", io_hdr.sbp[ata_shift+12]);
          fprintf(stdout, "status= %02X\n", io_hdr.sbp[ata_shift+13]);
        }
      }
    }

    // if verbosity level 7 then print the buffer to standard out
    // only use this for small reads!
    if (verbosity > 6 && !ata_inquiry)
    {
      fprintf (stdout, "sector=%lld\n", sector_start);
      fprintf (stdout, "count=%lld\n", sector_count);
      fprintf (stdout, "buffersize=%d\n", buffer_size);
      unsigned char *c;
      int i;
      for (i = 0; i < buffer_size; i+=16)
      {
        fprintf (stdout, "%04X:", i);
        int n;
        for (n=0; n < 16; n++)
        {
          c = (unsigned char *)buf+i+n;
          fprintf (stdout, "%02X ", *c);
        }
        for (n=0; n < 16; n++)
        {
          c = (unsigned char *)buf+i+n;
          fprintf (stdout, "%c", isprint(*c) ? *c : '.');
        }
        fprintf (stdout, "\n");
      }
      fprintf (stdout, "\n");
    }


    // ata buffer words have bytes flipped
  if ((verbosity > 6) && (ata_inquiry))
  {
    fprintf (stdout, "sector=%lld\n", sector_start);
    fprintf (stdout, "count=%lld\n", sector_count);
    fprintf (stdout, "buffersize=%d\n", buffer_size);
    unsigned char *c;
    unsigned char *d;
    int i;
    for (i = 0; i < buffer_size; i+=16)
    {
      fprintf (stdout, "%d:", i/2);
      int n;
      for (n=0; n < 16; n+=2)
      {
        c = (unsigned char *)buf+i+n;
        d = (unsigned char *)buf+i+n+1;
        fprintf (stdout, "%02X%02X ", *d, *c);
      }
      for (n=0; n < 16; n+=2)
      {
        c = (unsigned char *)buf+i+n;
        d = (unsigned char *)buf+i+n+1;
        fprintf (stdout, "%c%c", isprint(*d) ? *d : '.', isprint(*c) ? *c : '.');
      }
      fprintf (stdout, "\n");
    }
    fprintf (stdout, "\n");
  }


    // if sense_key is not 0, 1, or 3 then something bad happened
    if (sense_key != 0 && sense_key != 1 && sense_key != 3)
    {
      fprintf (stderr, "\nscsi sense key reports the command failed,\n");
      fprintf (stderr, "the command my not be supported, or something else went wrong\n");
      fprintf(stderr, "additional sense info: %02X %02X %02X\n", sense_key, asc, ascq);
      if (sense_key == 1)
      {
        fprintf (stderr, "sense key \'%02X\' indicates RECOVERED_ERROR\n", sense_key);
      }
      else if (sense_key == 2)
      {
        fprintf (stderr, "sense key \'%02X\' indicates NOT_READY\n", sense_key);
      }
      else if (sense_key == 3)
      {
        fprintf (stderr, "sense key \'%02X\' indicates MEDIUM_ERROR\n", sense_key);
      }
      else if (sense_key == 4)
      {
        fprintf (stderr, "sense key \'%02X\' indicates HARDWARE_ERROR\n", sense_key);
      }
      else if (sense_key == 5)
      {
        fprintf (stderr, "sense key \'%02X\' indicates ILLEGAL_REQUEST\n", sense_key);
      }
      else if (sense_key == 6)
      {
        fprintf (stderr, "sense key \'%02X\' indicates UNIT_ATTENTION\n", sense_key);
      }
      else if (sense_key == 7)
      {
        fprintf (stderr, "sense key \'%02X\' indicates DATA_PROTECT\n", sense_key);
      }
      else if (sense_key == 8)
      {
        fprintf (stderr, "sense key \'%02X\' indicates BLANK_CHECK\n", sense_key);
      }
      else if (sense_key == 9)
      {
        fprintf (stderr, "sense key \'%02X\' indicates VENDOR_SPECIFIC\n", sense_key);
      }
      else if (sense_key == 10)
      {
        fprintf (stderr, "sense key \'%02X\' indicates COPY_ABORTED\n", sense_key);
      }
      else if (sense_key == 11)
      {
        fprintf (stderr, "sense key \'%02X\' indicates ABORTED_COMMAND\n", sense_key);
      }
      else if (sense_key == 13)
      {
        fprintf (stderr, "sense key \'%02X\' indicates VOLUME_OVERFLOW\n", sense_key);
      }
      else if (sense_key == 14)
      {
        fprintf (stderr, "sense key \'%02X\' indicates MISCOMPARE\n", sense_key);
      }
      else if (sense_key == 15)
      {
        fprintf (stderr, "sense key \'%02X\' indicates COMPLETED\n", sense_key);
      }
      else
      {
        fprintf (stderr, "sense key \'%02X\' indicates Other (Unknown) Error\n", sense_key);
      }

      if (ata)
      {
        int ata_shift = 8;
        fprintf(stderr, "ATA return data:\n");
        fprintf(stderr, "descriptor= %02X\n", io_hdr.sbp[ata_shift+0]);
        fprintf(stderr, "additional length= %02X\n", io_hdr.sbp[ata_shift+1]);
        fprintf(stderr, "extend= %02X\n", io_hdr.sbp[ata_shift+2]);
        fprintf(stderr, "error= %02X\n", io_hdr.sbp[ata_shift+3]);
        fprintf(stderr, "count= %02X%02X\n", io_hdr.sbp[ata_shift+4], io_hdr.sbp[ata_shift+5]);
        fprintf(stderr, "LBAhigh= %02X%02X\n", io_hdr.sbp[ata_shift+10], io_hdr.sbp[ata_shift+8]);
        fprintf(stderr, "LBAmid= %02X%02X\n", io_hdr.sbp[ata_shift+6], io_hdr.sbp[ata_shift+11]);
        fprintf(stderr, "LBAlow= %02X%02X\n", io_hdr.sbp[ata_shift+9], io_hdr.sbp[ata_shift+7]);
        fprintf(stderr, "device= %02X\n", io_hdr.sbp[ata_shift+12]);
        fprintf(stderr, "status= %02X\n", io_hdr.sbp[ata_shift+13]);
      }
      fprintf (stderr, "\n\n\n\n");
      passthrough_error = 1;
      bad_ata_read = true;
    }
    return (0);
  }






#else

const char * device_id( const int ) { return 0; }

void print_sgio_help( void ) {}

void option_scsi_passthrough( void )
  {
    std::fprintf(stderr, "Error! The scsi_passthrough option is not available.\n" );
    std::fprintf(stderr, "You need to run configure with the --enable-linux option,\n" );
    std::fprintf(stderr, "and then recompile with make.\n" );
    std::exit( 1 );
  }

void option_ata_passthrough( void )
  {
    std::fprintf(stderr, "Error! The ata_passthrough option is not available.\n" );
    std::fprintf(stderr, "You need to run configure with the --enable-linux option,\n" );
    std::fprintf(stderr, "and then recompile with make.\n" );
    std::exit( 1 );
  }

void option_mark_abnormal_error( void )
  {
    std::fprintf(stderr, "Error! The mark_abnormal_error option is not available.\n" );
    std::fprintf(stderr, "You need to run configure with the --enable-linux option,\n" );
    std::fprintf(stderr, "and then recompile with make.\n" );
    std::exit( 1 );
  }

int check_device( const char *, const int, const int ) { return 0; }

int read_linux( const int fd, uint8_t * const buf, const int size, const long long, int sz, const int )
  {
    const int n = read( fd, buf + sz, size - sz );
    return n;
  }

#endif
