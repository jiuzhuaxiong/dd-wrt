/*
 * USB ViCam WebCam driver
 * Copyright (c) 2002 Joe Burks (jburks@wavicle.org),
 *                    Christopher L Cheney (ccheney@cheney.cx),
 *                    Pavel Machek (pavel@suse.cz),
 *                    John Tyner (jtyner@cs.ucr.edu),
 *                    Monroe Williams (monroe@pobox.com)
 *
 * Supports 3COM HomeConnect PC Digital WebCam
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * This source code is based heavily on the CPiA webcam driver which was
 * written by Peter Pregler, Scott J. Bertin and Johannes Erdfelt
 *
 * Portions of this code were also copied from usbvideo.c
 *
 * Special thanks to the the whole team at Sourceforge for help making
 * this driver become a reality.  Notably:
 * Andy Armstrong who reverse engineered the color encoding and
 * Pavel Machek and Chris Cheney who worked on reverse engineering the
 *    camera controls and wrote the first generation driver.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/videodev.h>
#include <linux/usb.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include "usbvideo.h"

// #define VICAM_DEBUG

#ifdef VICAM_DEBUG
#define ADBG(lineno,fmt,args...) printk(fmt, jiffies, __FUNCTION__, lineno, ##args)
#define DBG(fmt,args...) ADBG((__LINE__),KERN_DEBUG __FILE__"(%ld):%s (%d):"fmt,##args)
#else
#define DBG(fmn,args...) do {} while(0)
#endif

#define DRIVER_AUTHOR           "Joe Burks, jburks@wavicle.org"
#define DRIVER_DESC             "ViCam WebCam Driver"

/* Define these values to match your device */
#define USB_VICAM_VENDOR_ID	0x04c1
#define USB_VICAM_PRODUCT_ID	0x009d

#define VICAM_BYTES_PER_PIXEL   3
#define VICAM_MAX_READ_SIZE     (512*242+128)
#define VICAM_MAX_FRAME_SIZE    (VICAM_BYTES_PER_PIXEL*320*240)
#define VICAM_FRAMES            2

#define VICAM_HEADER_SIZE       64

#define clamp( x, l, h )        max_t( __typeof__( x ),         \
				       ( l ),                   \
				       min_t( __typeof__( x ),  \
					      ( h ),            \
					      ( x ) ) )

/* Not sure what all the bytes in these char
 * arrays do, but they're necessary to make
 * the camera work.
 */

static unsigned char setup1[] = {
	0xB6, 0xC3, 0x1F, 0x00, 0x02, 0x64, 0xE7, 0x67,
	0xFD, 0xFF, 0x0E, 0xC0, 0xE7, 0x09, 0xDE, 0x00,
	0x8E, 0x00, 0xC0, 0x09, 0x40, 0x03, 0xC0, 0x17,
	0x44, 0x03, 0x4B, 0xAF, 0xC0, 0x07, 0x00, 0x00,
	0x4B, 0xAF, 0x97, 0xCF, 0x00, 0x00
};

static unsigned char setup2[] = {
	0xB6, 0xC3, 0x03, 0x00, 0x03, 0x64, 0x18, 0x00,
	0x00, 0x00
};

static unsigned char setup3[] = {
	0xB6, 0xC3, 0x01, 0x00, 0x06, 0x64, 0x00, 0x00
};

static unsigned char setup4[] = {
	0xB6, 0xC3, 0x8F, 0x06, 0x02, 0x64, 0xE7, 0x07,
	0x00, 0x00, 0x08, 0xC0, 0xE7, 0x07, 0x00, 0x00,
	0x3E, 0xC0, 0xE7, 0x07, 0x54, 0x01, 0xAA, 0x00,
	0xE7, 0x07, 0xC8, 0x05, 0xB6, 0x00, 0xE7, 0x07,
	0x42, 0x01, 0xD2, 0x00, 0xE7, 0x07, 0x7C, 0x00,
	0x16, 0x00, 0xE7, 0x07, 0x56, 0x00, 0x18, 0x00,
	0xE7, 0x07, 0x06, 0x00, 0x92, 0xC0, 0xE7, 0x07,
	0x00, 0x00, 0x1E, 0xC0, 0xE7, 0x07, 0xFF, 0xFF,
	0x22, 0xC0, 0xE7, 0x07, 0x04, 0x00, 0x24, 0xC0,
	0xE7, 0x07, 0xEC, 0x27, 0x28, 0xC0, 0xE7, 0x07,
	0x16, 0x01, 0x8E, 0x00, 0xE7, 0x87, 0x01, 0x00,
	0x0E, 0xC0, 0x97, 0xCF, 0xD7, 0x09, 0x00, 0xC0,
	0xE7, 0x77, 0x01, 0x00, 0x92, 0xC0, 0x09, 0xC1,
	0xE7, 0x09, 0xFE, 0x05, 0x24, 0x01, 0xE7, 0x09,
	0x04, 0x06, 0x26, 0x01, 0xE7, 0x07, 0x07, 0x00,
	0x92, 0xC0, 0xE7, 0x05, 0x00, 0xC0, 0xC0, 0xDF,
	0x97, 0xCF, 0x17, 0x00, 0x57, 0x00, 0x17, 0x02,
	0xD7, 0x09, 0x00, 0xC0, 0xE7, 0x77, 0x01, 0x00,
	0x92, 0xC0, 0x0A, 0xC1, 0xE7, 0x57, 0xFF, 0xFF,
	0xFA, 0x05, 0x0D, 0xC0, 0xE7, 0x57, 0x00, 0x00,
	0xFA, 0x05, 0x0F, 0xC0, 0x9F, 0xAF, 0xC6, 0x00,
	0xE7, 0x05, 0x00, 0xC0, 0xC8, 0x05, 0xC1, 0x05,
	0xC0, 0x05, 0xC0, 0xDF, 0x97, 0xCF, 0x27, 0xDA,
	0xFA, 0x05, 0xEF, 0x07, 0x01, 0x00, 0x0B, 0x06,
	0x73, 0xCF, 0x9F, 0xAF, 0x78, 0x01, 0x9F, 0xAF,
	0x1A, 0x03, 0x6E, 0xCF, 0xE7, 0x09, 0xFC, 0x05,
	0x24, 0x01, 0xE7, 0x09, 0x02, 0x06, 0x26, 0x01,
	0xE7, 0x07, 0x07, 0x00, 0x92, 0xC0, 0xE7, 0x09,
	0xFC, 0x05, 0xFE, 0x05, 0xE7, 0x09, 0x02, 0x06,
	0x04, 0x06, 0xE7, 0x09, 0x00, 0x06, 0xFC, 0x05,
	0xE7, 0x09, 0xFE, 0x05, 0x00, 0x06, 0x27, 0xDA,
	0xFA, 0x05, 0xE7, 0x57, 0x01, 0x00, 0xFA, 0x05,
	0x02, 0xCA, 0x04, 0xC0, 0x97, 0xCF, 0x9F, 0xAF,
	0x66, 0x05, 0x97, 0xCF, 0xE7, 0x07, 0x40, 0x00,
	0x02, 0x06, 0xC8, 0x09, 0xFC, 0x05, 0x9F, 0xAF,
	0xDA, 0x02, 0x97, 0xCF, 0xCF, 0x17, 0x02, 0x00,
	0xEF, 0x57, 0x81, 0x00, 0x09, 0x06, 0x9F, 0xA0,
	0xB6, 0x01, 0xEF, 0x57, 0x80, 0x00, 0x09, 0x06,
	0x9F, 0xA0, 0x40, 0x02, 0xEF, 0x57, 0x01, 0x00,
	0x0B, 0x06, 0x9F, 0xA0, 0x46, 0x03, 0xE7, 0x07,
	0x01, 0x00, 0x0A, 0xC0, 0x46, 0xAF, 0x47, 0xAF,
	0x9F, 0xAF, 0x40, 0x02, 0xE7, 0x07, 0x2E, 0x00,
	0x0A, 0xC0, 0xEF, 0x87, 0x80, 0x00, 0x09, 0x06,
	0x97, 0xCF, 0x00, 0x0E, 0x01, 0x00, 0xC0, 0x57,
	0x51, 0x00, 0x9F, 0xC0, 0x9E, 0x02, 0xC0, 0x57,
	0x50, 0x00, 0x20, 0xC0, 0xC0, 0x57, 0x55, 0x00,
	0x12, 0xC0, 0xC0, 0x57, 0x56, 0x00, 0x9F, 0xC0,
	0x72, 0x02, 0x9F, 0xCF, 0xD6, 0x02, 0xC1, 0x0B,
	0x08, 0x06, 0x01, 0xD0, 0x6F, 0x90, 0x08, 0x06,
	0xC0, 0x07, 0x08, 0x00, 0xC1, 0x0B, 0x08, 0x06,
	0x9F, 0xAF, 0x28, 0x05, 0x97, 0xCF, 0x2F, 0x0E,
	0x02, 0x00, 0x08, 0x06, 0xC0, 0x07, 0x08, 0x00,
	0xC1, 0x0B, 0x08, 0x06, 0x9F, 0xAF, 0x28, 0x05,
	0x9F, 0xCF, 0xD6, 0x02, 0x2F, 0x0E, 0x02, 0x00,
	0x09, 0x06, 0xEF, 0x87, 0x80, 0x00, 0x09, 0x06,
	0x9F, 0xCF, 0xD6, 0x02, 0xEF, 0x67, 0x7F, 0xFF,
	0x09, 0x06, 0xE7, 0x67, 0xFF, 0xFD, 0x22, 0xC0,
	0xE7, 0x67, 0xEF, 0xFF, 0x24, 0xC0, 0xE7, 0x87,
	0x10, 0x00, 0x28, 0xC0, 0x9F, 0xAF, 0xB8, 0x05,
	0xE7, 0x87, 0xE0, 0x21, 0x24, 0xC0, 0x9F, 0xAF,
	0xA8, 0x05, 0xE7, 0x87, 0x08, 0x00, 0x24, 0xC0,
	0xE7, 0x67, 0xDF, 0xFF, 0x24, 0xC0, 0xC8, 0x07,
	0x0A, 0x00, 0xC0, 0x07, 0x00, 0x00, 0xC1, 0x07,
	0x01, 0x00, 0x9F, 0xAF, 0x28, 0x05, 0x9F, 0xAF,
	0xB8, 0x05, 0xC0, 0x07, 0x9E, 0x00, 0x9F, 0xAF,
	0x44, 0x05, 0xE7, 0x67, 0xFF, 0xFE, 0x24, 0xC0,
	0xC0, 0x09, 0x20, 0xC0, 0xE7, 0x87, 0x00, 0x01,
	0x24, 0xC0, 0xC0, 0x77, 0x00, 0x02, 0x0F, 0xC1,
	0xE7, 0x67, 0xF7, 0xFF, 0x24, 0xC0, 0xE7, 0x67,
	0xF7, 0xFF, 0x24, 0xC0, 0xE7, 0x87, 0x08, 0x00,
	0x24, 0xC0, 0x08, 0xDA, 0x5E, 0xC1, 0xEF, 0x07,
	0x80, 0x00, 0x09, 0x06, 0x97, 0xCF, 0xEF, 0x07,
	0x01, 0x00, 0x0A, 0x06, 0x97, 0xCF, 0xEF, 0x07,
	0x00, 0x00, 0x0B, 0x06, 0xEF, 0x07, 0x00, 0x00,
	0x0A, 0x06, 0xEF, 0x67, 0x7F, 0xFF, 0x09, 0x06,
	0xEF, 0x07, 0x00, 0x00, 0x0D, 0x06, 0xE7, 0x67,
	0xEF, 0xFF, 0x28, 0xC0, 0xE7, 0x67, 0x17, 0xD8,
	0x24, 0xC0, 0xE7, 0x07, 0x00, 0x00, 0x1E, 0xC0,
	0xE7, 0x07, 0xFF, 0xFF, 0x22, 0xC0, 0x97, 0xCF,
	0xC8, 0x07, 0x0E, 0x06, 0x9F, 0xAF, 0xDA, 0x02,
	0xE7, 0x07, 0x00, 0x00, 0xF2, 0x05, 0xE7, 0x07,
	0x10, 0x00, 0xF6, 0x05, 0xE7, 0x07, 0x0E, 0x06,
	0xF4, 0x05, 0xE7, 0x07, 0xD6, 0x02, 0xF8, 0x05,
	0xC8, 0x07, 0xF2, 0x05, 0xC1, 0x07, 0x00, 0x80,
	0x50, 0xAF, 0x97, 0xCF, 0x2F, 0x0C, 0x02, 0x00,
	0x07, 0x06, 0x2F, 0x0C, 0x04, 0x00, 0x06, 0x06,
	0xE7, 0x07, 0x00, 0x00, 0xF2, 0x05, 0xE7, 0x07,
	0x10, 0x00, 0xF6, 0x05, 0xE7, 0x07, 0xE2, 0x05,
	0xF4, 0x05, 0xE7, 0x07, 0xCE, 0x02, 0xF8, 0x05,
	0xC8, 0x07, 0xF2, 0x05, 0xC1, 0x07, 0x00, 0x80,
	0x51, 0xAF, 0x97, 0xCF, 0x9F, 0xAF, 0x66, 0x04,
	0x9F, 0xAF, 0x1A, 0x03, 0x59, 0xAF, 0x97, 0xCF,
	0xC0, 0x07, 0x0E, 0x00, 0xC1, 0x0B, 0x0C, 0x06,
	0x41, 0xD1, 0x9F, 0xAF, 0x28, 0x05, 0xC0, 0x07,
	0x3C, 0x00, 0x9F, 0xAF, 0x44, 0x05, 0x68, 0x00,
	0xC0, 0x07, 0x3B, 0x00, 0x9F, 0xAF, 0x44, 0x05,
	0x6F, 0x00, 0x0C, 0x06, 0x68, 0x00, 0xE0, 0x07,
	0x04, 0x01, 0xE8, 0x0B, 0x0A, 0x06, 0xE8, 0x07,
	0x00, 0x00, 0xE0, 0x07, 0x00, 0x02, 0xE0, 0x07,
	0xEC, 0x01, 0xE0, 0x07, 0xFC, 0xFF, 0x97, 0xCF,
	0xE7, 0x07, 0xFF, 0xFF, 0xFA, 0x05, 0xEF, 0x07,
	0x00, 0x00, 0x0B, 0x06, 0xE7, 0x07, 0x0E, 0x06,
	0x24, 0x01, 0xE7, 0x07, 0x0E, 0x06, 0xFE, 0x05,
	0xE7, 0x07, 0x40, 0x00, 0x26, 0x01, 0xE7, 0x07,
	0x40, 0x00, 0x04, 0x06, 0xE7, 0x07, 0x07, 0x00,
	0x92, 0xC0, 0x97, 0xCF, 0xEF, 0x07, 0x02, 0x00,
	0x0B, 0x06, 0x9F, 0xAF, 0x78, 0x01, 0xEF, 0x77,
	0x80, 0x00, 0x07, 0x06, 0x9F, 0xC0, 0x14, 0x04,
	0xEF, 0x77, 0x01, 0x00, 0x07, 0x06, 0x37, 0xC0,
	0xEF, 0x77, 0x01, 0x00, 0x0D, 0x06, 0x0F, 0xC1,
	0xEF, 0x07, 0x01, 0x00, 0x0D, 0x06, 0xC0, 0x07,
	0x02, 0x00, 0xC1, 0x07, 0x30, 0x00, 0x9F, 0xAF,
	0x28, 0x05, 0xC0, 0x07, 0x01, 0x00, 0xC1, 0x07,
	0x02, 0x00, 0x9F, 0xAF, 0x28, 0x05, 0xC8, 0x07,
	0xFF, 0x4F, 0x9F, 0xAF, 0xA8, 0x05, 0xC0, 0x07,
	0x38, 0x00, 0x9F, 0xAF, 0x44, 0x05, 0xC1, 0x77,
	0x03, 0x00, 0x02, 0xC1, 0x08, 0xDA, 0x75, 0xC1,
	0xC1, 0x77, 0x01, 0x00, 0x0A, 0xC1, 0xC0, 0x07,
	0x01, 0x00, 0xC1, 0x07, 0x02, 0x00, 0x9F, 0xAF,
	0x28, 0x05, 0xEF, 0x07, 0x01, 0x00, 0x06, 0x06,
	0x2C, 0xCF, 0xC0, 0x07, 0x01, 0x00, 0xC1, 0x07,
	0x04, 0x00, 0x9F, 0xAF, 0x28, 0x05, 0xEF, 0x07,
	0x00, 0x00, 0x06, 0x06, 0x22, 0xCF, 0xEF, 0x07,
	0x00, 0x00, 0x0D, 0x06, 0xEF, 0x57, 0x01, 0x00,
	0x06, 0x06, 0x1B, 0xC0, 0xC0, 0x07, 0x01, 0x00,
	0xC1, 0x07, 0x01, 0x00, 0x9F, 0xAF, 0x28, 0x05,
	0xC0, 0x07, 0x02, 0x00, 0xC1, 0x07, 0x30, 0x00,
	0x9F, 0xAF, 0x28, 0x05, 0xC8, 0x07, 0xFF, 0x4F,
	0x9F, 0xAF, 0xA8, 0x05, 0xC0, 0x07, 0x38, 0x00,
	0x9F, 0xAF, 0x44, 0x05, 0xC1, 0x67, 0x03, 0x00,
	0xC1, 0x57, 0x03, 0x00, 0x02, 0xC0, 0x08, 0xDA,
	0x73, 0xC1, 0xC0, 0x07, 0x02, 0x00, 0xC1, 0x07,
	0x12, 0x00, 0xEF, 0x57, 0x00, 0x00, 0x06, 0x06,
	0x02, 0xC0, 0xC1, 0x07, 0x23, 0x00, 0x9F, 0xAF,
	0x28, 0x05, 0xC0, 0x07, 0x14, 0x00, 0xC1, 0x0B,
	0xEA, 0x05, 0x9F, 0xAF, 0x28, 0x05, 0xC0, 0x07,
	0x3E, 0x00, 0x9F, 0xAF, 0x0A, 0x05, 0xE7, 0x09,
	0xE4, 0x05, 0xFA, 0x05, 0x27, 0xD8, 0xFA, 0x05,
	0xE7, 0x07, 0x0E, 0x06, 0xFC, 0x05, 0xE7, 0x07,
	0x4E, 0x06, 0x00, 0x06, 0xE7, 0x07, 0x40, 0x00,
	0x02, 0x06, 0x9F, 0xAF, 0x66, 0x05, 0x9F, 0xAF,
	0xC6, 0x00, 0x97, 0xCF, 0xC1, 0x0B, 0xE2, 0x05,
	0x41, 0xD0, 0x01, 0xD2, 0xC1, 0x17, 0x23, 0x00,
	0x9F, 0xAF, 0xDC, 0x04, 0xC0, 0x07, 0x04, 0x00,
	0xC1, 0x0B, 0xE3, 0x05, 0x9F, 0xAF, 0x28, 0x05,
	0xC0, 0x07, 0x06, 0x00, 0xC1, 0x09, 0xE6, 0x05,
	0x9F, 0xAF, 0x28, 0x05, 0xC0, 0x07, 0x07, 0x00,
	0xC1, 0x09, 0xE6, 0x05, 0xC1, 0xD1, 0x9F, 0xAF,
	0x28, 0x05, 0xC0, 0x07, 0x0B, 0x00, 0xC1, 0x09,
	0xE8, 0x05, 0x9F, 0xAF, 0x28, 0x05, 0xC0, 0x07,
	0x0C, 0x00, 0xC1, 0x09, 0xE8, 0x05, 0xC1, 0xD1,
	0x9F, 0xAF, 0x28, 0x05, 0xC0, 0x07, 0x0D, 0x00,
	0xC1, 0x07, 0x09, 0x00, 0x9F, 0xAF, 0x28, 0x05,
	0xC0, 0x07, 0x03, 0x00, 0xC1, 0x07, 0x32, 0x00,
	0x9F, 0xAF, 0x28, 0x05, 0xC0, 0x07, 0x0F, 0x00,
	0xC1, 0x07, 0x00, 0x00, 0x9F, 0xAF, 0x28, 0x05,
	0x97, 0xCF, 0xE7, 0x67, 0xFF, 0xD9, 0x24, 0xC0,
	0xC8, 0x07, 0x0A, 0x00, 0x40, 0x00, 0xC0, 0x67,
	0x00, 0x02, 0x27, 0x80, 0x24, 0xC0, 0xE7, 0x87,
	0x00, 0x04, 0x24, 0xC0, 0xE7, 0x67, 0xFF, 0xF9,
	0x24, 0xC0, 0x01, 0xD2, 0x08, 0xDA, 0x72, 0xC1,
	0xE7, 0x87, 0x00, 0x20, 0x24, 0xC0, 0x97, 0xCF,
	0x27, 0x00, 0x1E, 0xC0, 0xE7, 0x87, 0xFF, 0x00,
	0x22, 0xC0, 0xE7, 0x67, 0x7F, 0xFF, 0x24, 0xC0,
	0xE7, 0x87, 0x80, 0x00, 0x24, 0xC0, 0xE7, 0x87,
	0x80, 0x00, 0x24, 0xC0, 0x97, 0xCF, 0x9F, 0xAF,
	0x0A, 0x05, 0x67, 0x00, 0x1E, 0xC0, 0xE7, 0x67,
	0xBF, 0xFF, 0x24, 0xC0, 0xE7, 0x87, 0x40, 0x00,
	0x24, 0xC0, 0xE7, 0x87, 0x40, 0x00, 0x24, 0xC0,
	0x97, 0xCF, 0x9F, 0xAF, 0x0A, 0x05, 0xE7, 0x67,
	0x00, 0xFF, 0x22, 0xC0, 0xE7, 0x67, 0xFF, 0xFE,
	0x24, 0xC0, 0xE7, 0x67, 0xFF, 0xFE, 0x24, 0xC0,
	0xC1, 0x09, 0x20, 0xC0, 0xE7, 0x87, 0x00, 0x01,
	0x24, 0xC0, 0x97, 0xCF, 0xC0, 0x07, 0x40, 0x00,
	0xC8, 0x09, 0xFC, 0x05, 0xE7, 0x67, 0x00, 0xFF,
	0x22, 0xC0, 0xE7, 0x67, 0xFF, 0xFE, 0x24, 0xC0,
	0xE7, 0x67, 0xBF, 0xFF, 0x24, 0xC0, 0xE7, 0x67,
	0xBF, 0xFF, 0x24, 0xC0, 0x00, 0xDA, 0xE8, 0x09,
	0x20, 0xC0, 0xE7, 0x87, 0x40, 0x00, 0x24, 0xC0,
	0xE7, 0x87, 0x40, 0x00, 0x24, 0xC0, 0x00, 0xDA,
	0xE8, 0x09, 0x20, 0xC0, 0x6D, 0xC1, 0xE7, 0x87,
	0x00, 0x01, 0x24, 0xC0, 0x97, 0xCF, 0xE7, 0x07,
	0x32, 0x00, 0x12, 0xC0, 0xE7, 0x77, 0x00, 0x80,
	0x12, 0xC0, 0x7C, 0xC0, 0x97, 0xCF, 0xE7, 0x07,
	0x20, 0x4E, 0x12, 0xC0, 0xE7, 0x77, 0x00, 0x80,
	0x12, 0xC0, 0x7C, 0xC0, 0x97, 0xCF, 0x09, 0x02,
	0x19, 0x00, 0x01, 0x01, 0x00, 0x80, 0x96, 0x09,
	0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static unsigned char setup5[] = {
	0xB6, 0xC3, 0x2F, 0x01, 0x03, 0x64, 0x0E, 0x00,
	0x14, 0x00, 0x1A, 0x00, 0x20, 0x00, 0x26, 0x00,
	0x4A, 0x00, 0x64, 0x00, 0x6A, 0x00, 0x92, 0x00,
	0x9A, 0x00, 0xA0, 0x00, 0xB2, 0x00, 0xB8, 0x00,
	0xBE, 0x00, 0xC2, 0x00, 0xC8, 0x00, 0xCE, 0x00,
	0xDC, 0x00, 0xDA, 0x00, 0xE2, 0x00, 0xE0, 0x00,
	0xE8, 0x00, 0xE6, 0x00, 0xEE, 0x00, 0xEC, 0x00,
	0xF2, 0x00, 0xF8, 0x00, 0x02, 0x01, 0x0A, 0x01,
	0x0E, 0x01, 0x12, 0x01, 0x1E, 0x01, 0x22, 0x01,
	0x28, 0x01, 0x2C, 0x01, 0x32, 0x01, 0x36, 0x01,
	0x44, 0x01, 0x50, 0x01, 0x5E, 0x01, 0x72, 0x01,
	0x76, 0x01, 0x7A, 0x01, 0x80, 0x01, 0x88, 0x01,
	0x8C, 0x01, 0x94, 0x01, 0x9C, 0x01, 0xA0, 0x01,
	0xA4, 0x01, 0xAA, 0x01, 0xB0, 0x01, 0xB4, 0x01,
	0xBA, 0x01, 0xD0, 0x01, 0xDA, 0x01, 0xF6, 0x01,
	0xFA, 0x01, 0x02, 0x02, 0x34, 0x02, 0x3C, 0x02,
	0x44, 0x02, 0x4A, 0x02, 0x50, 0x02, 0x56, 0x02,
	0x74, 0x02, 0x78, 0x02, 0x7E, 0x02, 0x84, 0x02,
	0x8A, 0x02, 0x88, 0x02, 0x90, 0x02, 0x8E, 0x02,
	0x94, 0x02, 0xA2, 0x02, 0xA8, 0x02, 0xAE, 0x02,
	0xB4, 0x02, 0xBA, 0x02, 0xB8, 0x02, 0xC0, 0x02,
	0xBE, 0x02, 0xC4, 0x02, 0xD0, 0x02, 0xD4, 0x02,
	0xE0, 0x02, 0xE6, 0x02, 0xEE, 0x02, 0xF8, 0x02,
	0xFC, 0x02, 0x06, 0x03, 0x1E, 0x03, 0x24, 0x03,
	0x28, 0x03, 0x30, 0x03, 0x2E, 0x03, 0x3C, 0x03,
	0x4A, 0x03, 0x4E, 0x03, 0x54, 0x03, 0x58, 0x03,
	0x5E, 0x03, 0x66, 0x03, 0x6E, 0x03, 0x7A, 0x03,
	0x86, 0x03, 0x8E, 0x03, 0x96, 0x03, 0xB2, 0x03,
	0xB8, 0x03, 0xC6, 0x03, 0xCC, 0x03, 0xD4, 0x03,
	0xDA, 0x03, 0xE8, 0x03, 0xF4, 0x03, 0xFC, 0x03,
	0x04, 0x04, 0x20, 0x04, 0x2A, 0x04, 0x32, 0x04,
	0x36, 0x04, 0x3E, 0x04, 0x44, 0x04, 0x42, 0x04,
	0x48, 0x04, 0x4E, 0x04, 0x4C, 0x04, 0x54, 0x04,
	0x52, 0x04, 0x5A, 0x04, 0x5E, 0x04, 0x62, 0x04,
	0x68, 0x04, 0x74, 0x04, 0x7C, 0x04, 0x80, 0x04,
	0x88, 0x04, 0x8C, 0x04, 0x94, 0x04, 0x9A, 0x04,
	0xA2, 0x04, 0xA6, 0x04, 0xAE, 0x04, 0xB4, 0x04,
	0xC0, 0x04, 0xCC, 0x04, 0xD8, 0x04, 0x2A, 0x05,
	0x46, 0x05, 0x6C, 0x05, 0x00, 0x00
};

/* rvmalloc / rvfree copied from usbvideo.c
 *
 * Not sure why these are not yet non-statics which I can reference through
 * usbvideo.h the same as it is in 2.4.20.  I bet this will get fixed sometime
 * in the future.
 *
*/
static void *rvmalloc(unsigned long size)
{
	void *mem;
	unsigned long adr;

	size = PAGE_ALIGN(size);
	mem = vmalloc_32(size);
	if (!mem)
		return NULL;

	memset(mem, 0, size); /* Clear the ram out, no junk to the user */
	adr = (unsigned long) mem;
	while (size > 0) {
		SetPageReserved(vmalloc_to_page((void *)adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	return mem;
}

static void rvfree(void *mem, unsigned long size)
{
	unsigned long adr;

	if (!mem)
		return;

	adr = (unsigned long) mem;
	while ((long) size > 0) {
		ClearPageReserved(vmalloc_to_page((void *)adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	vfree(mem);
}

struct vicam_camera {
	u16 shutter_speed;	// capture shutter speed
	u16 gain;		// capture gain

	u8 *raw_image;		// raw data captured from the camera
	u8 *framebuf;		// processed data in RGB24 format
	u8 *cntrlbuf;		// area used to send control msgs

	struct video_device vdev;	// v4l video device
	struct usb_device *udev;	// usb device

	/* guard against simultaneous accesses to the camera */
	struct mutex cam_lock;

	int is_initialized;
	u8 open_count;
	u8 bulkEndpoint;
	int needsDummyRead;

#if defined(CONFIG_VIDEO_PROC_FS)
	struct proc_dir_entry *proc_dir;
#endif

};

static int vicam_probe( struct usb_interface *intf, const struct usb_device_id *id);
static void vicam_disconnect(struct usb_interface *intf);
static void read_frame(struct vicam_camera *cam, int framenum);
static void vicam_decode_color(const u8 *, u8 *);

static int __send_control_msg(struct vicam_camera *cam,
			      u8 request,
			      u16 value,
			      u16 index,
			      unsigned char *cp,
			      u16 size)
{
	int status;

	/* cp must be memory that has been allocated by kmalloc */

	status = usb_control_msg(cam->udev,
				 usb_sndctrlpipe(cam->udev, 0),
				 request,
				 USB_DIR_OUT | USB_TYPE_VENDOR |
				 USB_RECIP_DEVICE, value, index,
				 cp, size, 1000);

	status = min(status, 0);

	if (status < 0) {
		printk(KERN_INFO "Failed sending control message, error %d.\n",
		       status);
	}

	return status;
}

static int send_control_msg(struct vicam_camera *cam,
			    u8 request,
			    u16 value,
			    u16 index,
			    unsigned char *cp,
			    u16 size)
{
	int status = -ENODEV;
	mutex_lock(&cam->cam_lock);
	if (cam->udev) {
		status = __send_control_msg(cam, request, value,
					    index, cp, size);
	}
	mutex_unlock(&cam->cam_lock);
	return status;
}
static int
initialize_camera(struct vicam_camera *cam)
{
	const struct {
		u8 *data;
		u32 size;
	} firmware[] = {
		{ .data = setup1, .size = sizeof(setup1) },
		{ .data = setup2, .size = sizeof(setup2) },
		{ .data = setup3, .size = sizeof(setup3) },
		{ .data = setup4, .size = sizeof(setup4) },
		{ .data = setup5, .size = sizeof(setup5) },
		{ .data = setup3, .size = sizeof(setup3) },
		{ .data = NULL, .size = 0 }
	};

	int err, i;

	for (i = 0, err = 0; firmware[i].data && !err; i++) {
		memcpy(cam->cntrlbuf, firmware[i].data, firmware[i].size);

		err = send_control_msg(cam, 0xff, 0, 0,
				       cam->cntrlbuf, firmware[i].size);
	}

	return err;
}

static int
set_camera_power(struct vicam_camera *cam, int state)
{
	int status;

	if ((status = send_control_msg(cam, 0x50, state, 0, NULL, 0)) < 0)
		return status;

	if (state) {
		send_control_msg(cam, 0x55, 1, 0, NULL, 0);
	}

	return 0;
}

static int
vicam_ioctl(struct inode *inode, struct file *file, unsigned int ioctlnr, unsigned long arg)
{
	void __user *user_arg = (void __user *)arg;
	struct vicam_camera *cam = file->private_data;
	int retval = 0;

	if (!cam)
		return -ENODEV;

	switch (ioctlnr) {
		/* query capabilities */
	case VIDIOCGCAP:
		{
			struct video_capability b;

			DBG("VIDIOCGCAP\n");
			memset(&b, 0, sizeof(b));
			strcpy(b.name, "ViCam-based Camera");
			b.type = VID_TYPE_CAPTURE;
			b.channels = 1;
			b.audios = 0;
			b.maxwidth = 320;	/* VIDEOSIZE_CIF */
			b.maxheight = 240;
			b.minwidth = 320;	/* VIDEOSIZE_48_48 */
			b.minheight = 240;

			if (copy_to_user(user_arg, &b, sizeof(b)))
				retval = -EFAULT;

			break;
		}
		/* get/set video source - we are a camera and nothing else */
	case VIDIOCGCHAN:
		{
			struct video_channel v;

			DBG("VIDIOCGCHAN\n");
			if (copy_from_user(&v, user_arg, sizeof(v))) {
				retval = -EFAULT;
				break;
			}
			if (v.channel != 0) {
				retval = -EINVAL;
				break;
			}

			v.channel = 0;
			strcpy(v.name, "Camera");
			v.tuners = 0;
			v.flags = 0;
			v.type = VIDEO_TYPE_CAMERA;
			v.norm = 0;

			if (copy_to_user(user_arg, &v, sizeof(v)))
				retval = -EFAULT;
			break;
		}

	case VIDIOCSCHAN:
		{
			int v;

			if (copy_from_user(&v, user_arg, sizeof(v)))
				retval = -EFAULT;
			DBG("VIDIOCSCHAN %d\n", v);

			if (retval == 0 && v != 0)
				retval = -EINVAL;

			break;
		}

		/* image properties */
	case VIDIOCGPICT:
		{
			struct video_picture vp;
			DBG("VIDIOCGPICT\n");
			memset(&vp, 0, sizeof (struct video_picture));
			vp.brightness = cam->gain << 8;
			vp.depth = 24;
			vp.palette = VIDEO_PALETTE_RGB24;
			if (copy_to_user(user_arg, &vp, sizeof (struct video_picture)))
				retval = -EFAULT;
			break;
		}

	case VIDIOCSPICT:
		{
			struct video_picture vp;

			if (copy_from_user(&vp, user_arg, sizeof(vp))) {
				retval = -EFAULT;
				break;
			}

			DBG("VIDIOCSPICT depth = %d, pal = %d\n", vp.depth,
			    vp.palette);

			cam->gain = vp.brightness >> 8;

			if (vp.depth != 24
			    || vp.palette != VIDEO_PALETTE_RGB24)
				retval = -EINVAL;

			break;
		}

		/* get/set capture window */
	case VIDIOCGWIN:
		{
			struct video_window vw;
			vw.x = 0;
			vw.y = 0;
			vw.width = 320;
			vw.height = 240;
			vw.chromakey = 0;
			vw.flags = 0;
			vw.clips = NULL;
			vw.clipcount = 0;

			DBG("VIDIOCGWIN\n");

			if (copy_to_user(user_arg, (void *)&vw, sizeof(vw)))
				retval = -EFAULT;

			// I'm not sure what the deal with a capture window is, it is very poorly described
			// in the doc.  So I won't support it now.
			break;
		}

	case VIDIOCSWIN:
		{

			struct video_window vw;

			if (copy_from_user(&vw, user_arg, sizeof(vw))) {
				retval = -EFAULT;
				break;
			}

			DBG("VIDIOCSWIN %d x %d\n", vw.width, vw.height);

			if ( vw.width != 320 || vw.height != 240 )
				retval = -EFAULT;

			break;
		}

		/* mmap interface */
	case VIDIOCGMBUF:
		{
			struct video_mbuf vm;
			int i;

			DBG("VIDIOCGMBUF\n");
			memset(&vm, 0, sizeof (vm));
			vm.size =
			    VICAM_MAX_FRAME_SIZE * VICAM_FRAMES;
			vm.frames = VICAM_FRAMES;
			for (i = 0; i < VICAM_FRAMES; i++)
				vm.offsets[i] = VICAM_MAX_FRAME_SIZE * i;

			if (copy_to_user(user_arg, (void *)&vm, sizeof(vm)))
				retval = -EFAULT;

			break;
		}

	case VIDIOCMCAPTURE:
		{
			struct video_mmap vm;
			// int video_size;

			if (copy_from_user((void *)&vm, user_arg, sizeof(vm))) {
				retval = -EFAULT;
				break;
			}

			DBG("VIDIOCMCAPTURE frame=%d, height=%d, width=%d, format=%d.\n",vm.frame,vm.width,vm.height,vm.format);

			if ( vm.frame >= VICAM_FRAMES || vm.format != VIDEO_PALETTE_RGB24 )
				retval = -EINVAL;

			// in theory right here we'd start the image capturing
			// (fill in a bulk urb and submit it asynchronously)
			//
			// Instead we're going to do a total hack job for now and
			// retrieve the frame in VIDIOCSYNC

			break;
		}

	case VIDIOCSYNC:
		{
			int frame;

			if (copy_from_user((void *)&frame, user_arg, sizeof(int))) {
				retval = -EFAULT;
				break;
			}
			DBG("VIDIOCSYNC: %d\n", frame);

			read_frame(cam, frame);
			vicam_decode_color(cam->raw_image,
					   cam->framebuf +
					   frame * VICAM_MAX_FRAME_SIZE );

			break;
		}

		/* pointless to implement overlay with this camera */
	case VIDIOCCAPTURE:
	case VIDIOCGFBUF:
	case VIDIOCSFBUF:
	case VIDIOCKEY:
		retval = -EINVAL;
		break;

		/* tuner interface - we have none */
	case VIDIOCGTUNER:
	case VIDIOCSTUNER:
	case VIDIOCGFREQ:
	case VIDIOCSFREQ:
		retval = -EINVAL;
		break;

		/* audio interface - we have none */
	case VIDIOCGAUDIO:
	case VIDIOCSAUDIO:
		retval = -EINVAL;
		break;
	default:
		retval = -ENOIOCTLCMD;
		break;
	}

	return retval;
}

static int
vicam_open(struct inode *inode, struct file *file)
{
	struct video_device *dev = video_devdata(file);
	struct vicam_camera *cam =
	    (struct vicam_camera *) dev->priv;
	DBG("open\n");

	if (!cam) {
		printk(KERN_ERR
		       "vicam video_device improperly initialized");
		return -EINVAL;
	}

	/* the videodev_lock held above us protects us from
	 * simultaneous opens...for now. we probably shouldn't
	 * rely on this fact forever.
	 */

	if (cam->open_count > 0) {
		printk(KERN_INFO
		       "vicam_open called on already opened camera");
		return -EBUSY;
	}

	cam->raw_image = kmalloc(VICAM_MAX_READ_SIZE, GFP_KERNEL);
	if (!cam->raw_image) {
		return -ENOMEM;
	}

	cam->framebuf = rvmalloc(VICAM_MAX_FRAME_SIZE * VICAM_FRAMES);
	if (!cam->framebuf) {
		kfree(cam->raw_image);
		return -ENOMEM;
	}

	cam->cntrlbuf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!cam->cntrlbuf) {
		kfree(cam->raw_image);
		rvfree(cam->framebuf, VICAM_MAX_FRAME_SIZE * VICAM_FRAMES);
		return -ENOMEM;
	}

	// First upload firmware, then turn the camera on

	if (!cam->is_initialized) {
		initialize_camera(cam);

		cam->is_initialized = 1;
	}

	set_camera_power(cam, 1);

	cam->needsDummyRead = 1;
	cam->open_count++;

	file->private_data = cam;

	return 0;
}

static int
vicam_close(struct inode *inode, struct file *file)
{
	struct vicam_camera *cam = file->private_data;
	int open_count;
	struct usb_device *udev;

	DBG("close\n");

	/* it's not the end of the world if
	 * we fail to turn the camera off.
	 */

	set_camera_power(cam, 0);

	kfree(cam->raw_image);
	rvfree(cam->framebuf, VICAM_MAX_FRAME_SIZE * VICAM_FRAMES);
	kfree(cam->cntrlbuf);

	mutex_lock(&cam->cam_lock);

	cam->open_count--;
	open_count = cam->open_count;
	udev = cam->udev;

	mutex_unlock(&cam->cam_lock);

	if (!open_count && !udev) {
		kfree(cam);
	}

	return 0;
}

static void vicam_decode_color(const u8 *data, u8 *rgb)
{
	/* vicam_decode_color - Convert from Vicam Y-Cr-Cb to RGB
	 * Copyright (C) 2002 Monroe Williams (monroe@pobox.com)
	 */

	int i, prevY, nextY;

	prevY = 512;
	nextY = 512;

	data += VICAM_HEADER_SIZE;

	for( i = 0; i < 240; i++, data += 512 ) {
		const int y = ( i * 242 ) / 240;

		int j, prevX, nextX;
		int Y, Cr, Cb;

		if ( y == 242 - 1 ) {
			nextY = -512;
		}

		prevX = 1;
		nextX = 1;

		for ( j = 0; j < 320; j++, rgb += 3 ) {
			const int x = ( j * 512 ) / 320;
			const u8 * const src = &data[x];

			if ( x == 512 - 1 ) {
				nextX = -1;
			}

			Cr = ( src[prevX] - src[0] ) +
				( src[nextX] - src[0] );
			Cr /= 2;

			Cb = ( src[prevY] - src[prevX + prevY] ) +
				( src[prevY] - src[nextX + prevY] ) +
				( src[nextY] - src[prevX + nextY] ) +
				( src[nextY] - src[nextX + nextY] );
			Cb /= 4;

			Y = 1160 * ( src[0] + ( Cr / 2 ) - 16 );

			if ( i & 1 ) {
				int Ct = Cr;
				Cr = Cb;
				Cb = Ct;
			}

			if ( ( x ^ i ) & 1 ) {
				Cr = -Cr;
				Cb = -Cb;
			}

			rgb[0] = clamp( ( ( Y + ( 2017 * Cb ) ) +
					500 ) / 900, 0, 255 );
			rgb[1] = clamp( ( ( Y - ( 392 * Cb ) -
					  ( 813 * Cr ) ) +
					  500 ) / 1000, 0, 255 );
			rgb[2] = clamp( ( ( Y + ( 1594 * Cr ) ) +
					500 ) / 1300, 0, 255 );

			prevX = -1;
		}

		prevY = -512;
	}
}

static void
read_frame(struct vicam_camera *cam, int framenum)
{
	unsigned char *request = cam->cntrlbuf;
	int realShutter;
	int n;
	int actual_length;

	if (cam->needsDummyRead) {
		cam->needsDummyRead = 0;
		read_frame(cam, framenum);
	}

	memset(request, 0, 16);
	request[0] = cam->gain;	// 0 = 0% gain, FF = 100% gain

	request[1] = 0;	// 512x242 capture

	request[2] = 0x90;	// the function of these two bytes
	request[3] = 0x07;	// is not yet understood

	if (cam->shutter_speed > 60) {
		// Short exposure
		realShutter =
		    ((-15631900 / cam->shutter_speed) + 260533) / 1000;
		request[4] = realShutter & 0xFF;
		request[5] = (realShutter >> 8) & 0xFF;
		request[6] = 0x03;
		request[7] = 0x01;
	} else {
		// Long exposure
		realShutter = 15600 / cam->shutter_speed - 1;
		request[4] = 0;
		request[5] = 0;
		request[6] = realShutter & 0xFF;
		request[7] = realShutter >> 8;
	}

	// Per John Markus Bjørndalen, byte at index 8 causes problems if it isn't 0
	request[8] = 0;
	// bytes 9-15 do not seem to affect exposure or image quality

	mutex_lock(&cam->cam_lock);

	if (!cam->udev) {
		goto done;
	}

	n = __send_control_msg(cam, 0x51, 0x80, 0, request, 16);

	if (n < 0) {
		printk(KERN_ERR
		       " Problem sending frame capture control message");
		goto done;
	}

	n = usb_bulk_msg(cam->udev,
			 usb_rcvbulkpipe(cam->udev, cam->bulkEndpoint),
			 cam->raw_image,
			 512 * 242 + 128, &actual_length, 10000);

	if (n < 0) {
		printk(KERN_ERR "Problem during bulk read of frame data: %d\n",
		       n);
	}

 done:
	mutex_unlock(&cam->cam_lock);
}

static ssize_t
vicam_read( struct file *file, char __user *buf, size_t count, loff_t *ppos )
{
	struct vicam_camera *cam = file->private_data;

	DBG("read %d bytes.\n", (int) count);

	if (*ppos >= VICAM_MAX_FRAME_SIZE) {
		*ppos = 0;
		return 0;
	}

	if (*ppos == 0) {
		read_frame(cam, 0);
		vicam_decode_color(cam->raw_image,
				   cam->framebuf +
				   0 * VICAM_MAX_FRAME_SIZE);
	}

	count = min_t(size_t, count, VICAM_MAX_FRAME_SIZE - *ppos);

	if (copy_to_user(buf, &cam->framebuf[*ppos], count)) {
		count = -EFAULT;
	} else {
		*ppos += count;
	}

	if (count == VICAM_MAX_FRAME_SIZE) {
		*ppos = 0;
	}

	return count;
}


static int
vicam_mmap(struct file *file, struct vm_area_struct *vma)
{
	// TODO: allocate the raw frame buffer if necessary
	unsigned long page, pos;
	unsigned long start = vma->vm_start;
	unsigned long size  = vma->vm_end-vma->vm_start;
	struct vicam_camera *cam = file->private_data;

	if (!cam)
		return -ENODEV;

	DBG("vicam_mmap: %ld\n", size);

	/* We let mmap allocate as much as it wants because Linux was adding 2048 bytes
	 * to the size the application requested for mmap and it was screwing apps up.
	 if (size > VICAM_FRAMES*VICAM_MAX_FRAME_SIZE)
	 return -EINVAL;
	 */

	pos = (unsigned long)cam->framebuf;
	while (size > 0) {
		page = vmalloc_to_pfn((void *)pos);
		if (remap_pfn_range(vma, start, page, PAGE_SIZE, PAGE_SHARED))
			return -EAGAIN;

		start += PAGE_SIZE;
		pos += PAGE_SIZE;
		if (size > PAGE_SIZE)
			size -= PAGE_SIZE;
		else
			size = 0;
	}

	return 0;
}

#if defined(CONFIG_VIDEO_PROC_FS)

static struct proc_dir_entry *vicam_proc_root = NULL;

static int vicam_read_helper(char *page, char **start, off_t off,
				int count, int *eof, int value)
{
	char *out = page;
	int len;

	out += sprintf(out, "%d",value);

	len = out - page;
	len -= off;
	if (len < count) {
		*eof = 1;
		if (len <= 0)
			return 0;
	} else
		len = count;

	*start = page + off;
	return len;
}

static int vicam_read_proc_shutter(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	return vicam_read_helper(page,start,off,count,eof,
				((struct vicam_camera *)data)->shutter_speed);
}

static int vicam_read_proc_gain(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	return vicam_read_helper(page,start,off,count,eof,
				((struct vicam_camera *)data)->gain);
}

static int
vicam_write_proc_shutter(struct file *file, const char *buffer,
			 unsigned long count, void *data)
{
	u16 stmp;
	char kbuf[8];
	struct vicam_camera *cam = (struct vicam_camera *) data;

	if (count > 6)
		return -EINVAL;

	if (copy_from_user(kbuf, buffer, count))
		return -EFAULT;

	stmp = (u16) simple_strtoul(kbuf, NULL, 10);
	if (stmp < 4 || stmp > 32000)
		return -EINVAL;

	cam->shutter_speed = stmp;

	return count;
}

static int
vicam_write_proc_gain(struct file *file, const char *buffer,
		      unsigned long count, void *data)
{
	u16 gtmp;
	char kbuf[8];

	struct vicam_camera *cam = (struct vicam_camera *) data;

	if (count > 4)
		return -EINVAL;

	if (copy_from_user(kbuf, buffer, count))
		return -EFAULT;

	gtmp = (u16) simple_strtoul(kbuf, NULL, 10);
	if (gtmp > 255)
		return -EINVAL;
	cam->gain = gtmp;

	return count;
}

static void
vicam_create_proc_root(void)
{
	vicam_proc_root = proc_mkdir("video/vicam", NULL);

	if (vicam_proc_root)
		vicam_proc_root->owner = THIS_MODULE;
	else
		printk(KERN_ERR
		       "could not create /proc entry for vicam!");
}

static void
vicam_destroy_proc_root(void)
{
	if (vicam_proc_root)
		remove_proc_entry("video/vicam", 0);
}

static void
vicam_create_proc_entry(struct vicam_camera *cam)
{
	char name[64];
	struct proc_dir_entry *ent;

	DBG(KERN_INFO "vicam: creating proc entry\n");

	if (!vicam_proc_root || !cam) {
		printk(KERN_INFO
		       "vicam: could not create proc entry, %s pointer is null.\n",
		       (!cam ? "camera" : "root"));
		return;
	}

	sprintf(name, "video%d", cam->vdev.minor);

	cam->proc_dir = proc_mkdir(name, vicam_proc_root);

	if ( !cam->proc_dir )
		return; // FIXME: We should probably return an error here

	ent = create_proc_entry("shutter", S_IFREG | S_IRUGO | S_IWUSR,
				cam->proc_dir);
	if (ent) {
		ent->data = cam;
		ent->read_proc = vicam_read_proc_shutter;
		ent->write_proc = vicam_write_proc_shutter;
		ent->size = 64;
	}

	ent = create_proc_entry("gain", S_IFREG | S_IRUGO | S_IWUSR,
				cam->proc_dir);
	if (ent) {
		ent->data = cam;
		ent->read_proc = vicam_read_proc_gain;
		ent->write_proc = vicam_write_proc_gain;
		ent->size = 64;
	}
}

static void
vicam_destroy_proc_entry(void *ptr)
{
	struct vicam_camera *cam = (struct vicam_camera *) ptr;
	char name[16];

	if ( !cam->proc_dir )
		return;

	sprintf(name, "video%d", cam->vdev.minor);
	remove_proc_entry("shutter", cam->proc_dir);
	remove_proc_entry("gain", cam->proc_dir);
	remove_proc_entry(name,vicam_proc_root);
	cam->proc_dir = NULL;

}

#else
static inline void vicam_create_proc_root(void) { }
static inline void vicam_destroy_proc_root(void) { }
static inline void vicam_create_proc_entry(struct vicam_camera *cam) { }
static inline void vicam_destroy_proc_entry(void *ptr) { }
#endif

static struct file_operations vicam_fops = {
	.owner		= THIS_MODULE,
	.open		= vicam_open,
	.release	= vicam_close,
	.read		= vicam_read,
	.mmap		= vicam_mmap,
	.ioctl		= vicam_ioctl,
	.compat_ioctl	= v4l_compat_ioctl32,
	.llseek		= no_llseek,
};

static struct video_device vicam_template = {
	.owner 		= THIS_MODULE,
	.name 		= "ViCam-based USB Camera",
	.type 		= VID_TYPE_CAPTURE,
	.hardware 	= VID_HARDWARE_VICAM,
	.fops 		= &vicam_fops,
	.minor 		= -1,
};

/* table of devices that work with this driver */
static struct usb_device_id vicam_table[] = {
	{USB_DEVICE(USB_VICAM_VENDOR_ID, USB_VICAM_PRODUCT_ID)},
	{}			/* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, vicam_table);

static struct usb_driver vicam_driver = {
	.name		= "vicam",
	.probe		= vicam_probe,
	.disconnect	= vicam_disconnect,
	.id_table	= vicam_table
};

/**
 *	vicam_probe
 *	@intf: the interface
 *	@id: the device id
 *
 *	Called by the usb core when a new device is connected that it thinks
 *	this driver might be interested in.
 */
static int
vicam_probe( struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	int bulkEndpoint = 0;
	const struct usb_host_interface *interface;
	const struct usb_endpoint_descriptor *endpoint;
	struct vicam_camera *cam;

	printk(KERN_INFO "ViCam based webcam connected\n");

	interface = intf->cur_altsetting;

	DBG(KERN_DEBUG "Interface %d. has %u. endpoints!\n",
	       interface->desc.bInterfaceNumber, (unsigned) (interface->desc.bNumEndpoints));
	endpoint = &interface->endpoint[0].desc;

	if ((endpoint->bEndpointAddress & 0x80) &&
	    ((endpoint->bmAttributes & 3) == 0x02)) {
		/* we found a bulk in endpoint */
		bulkEndpoint = endpoint->bEndpointAddress;
	} else {
		printk(KERN_ERR
		       "No bulk in endpoint was found ?! (this is bad)\n");
	}

	if ((cam =
	     kmalloc(sizeof (struct vicam_camera), GFP_KERNEL)) == NULL) {
		printk(KERN_WARNING
		       "could not allocate kernel memory for vicam_camera struct\n");
		return -ENOMEM;
	}

	memset(cam, 0, sizeof (struct vicam_camera));

	cam->shutter_speed = 15;

	mutex_init(&cam->cam_lock);

	memcpy(&cam->vdev, &vicam_template,
	       sizeof (vicam_template));
	cam->vdev.priv = cam;	// sort of a reverse mapping for those functions that get vdev only

	cam->udev = dev;
	cam->bulkEndpoint = bulkEndpoint;

	if (video_register_device(&cam->vdev, VFL_TYPE_GRABBER, -1) == -1) {
		kfree(cam);
		printk(KERN_WARNING "video_register_device failed\n");
		return -EIO;
	}

	vicam_create_proc_entry(cam);

	printk(KERN_INFO "ViCam webcam driver now controlling video device %d\n",cam->vdev.minor);

	usb_set_intfdata (intf, cam);

	return 0;
}

static void
vicam_disconnect(struct usb_interface *intf)
{
	int open_count;
	struct vicam_camera *cam = usb_get_intfdata (intf);
	usb_set_intfdata (intf, NULL);

	/* we must unregister the device before taking its
	 * cam_lock. This is because the video open call
	 * holds the same lock as video unregister. if we
	 * unregister inside of the cam_lock and open also
	 * uses the cam_lock, we get deadlock.
	 */

	video_unregister_device(&cam->vdev);

	/* stop the camera from being used */

	mutex_lock(&cam->cam_lock);

	/* mark the camera as gone */

	cam->udev = NULL;

	vicam_destroy_proc_entry(cam);

	/* the only thing left to do is synchronize with
	 * our close/release function on who should release
	 * the camera memory. if there are any users using the
	 * camera, it's their job. if there are no users,
	 * it's ours.
	 */

	open_count = cam->open_count;

	mutex_unlock(&cam->cam_lock);

	if (!open_count) {
		kfree(cam);
	}

	printk(KERN_DEBUG "ViCam-based WebCam disconnected\n");
}

/*
 */
static int __init
usb_vicam_init(void)
{
	int retval;
	DBG(KERN_INFO "ViCam-based WebCam driver startup\n");
	vicam_create_proc_root();
	retval = usb_register(&vicam_driver);
	if (retval)
		printk(KERN_WARNING "usb_register failed!\n");
	return retval;
}

static void __exit
usb_vicam_exit(void)
{
	DBG(KERN_INFO
	       "ViCam-based WebCam driver shutdown\n");

	usb_deregister(&vicam_driver);
	vicam_destroy_proc_root();
}

module_init(usb_vicam_init);
module_exit(usb_vicam_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
