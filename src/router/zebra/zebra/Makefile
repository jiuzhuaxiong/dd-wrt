# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /usr/local/zebra
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/zebra
pkglibdir = $(libdir)/zebra
pkgincludedir = $(includedir)/zebra

top_builddir = ..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = i686-pc-linux-gnu
host_triplet = i686-pc-linux-gnu
AR = ar
BGPD = 
CC = gcc
CPP = gcc -E
CURSES = 
IF_METHOD = if_netlink.o
IF_PROC = if_proc.o
IPFORWARD = ipforward_proc.o
KERNEL_METHOD = 
LIBPAM = 
LIB_REGEX = 
MAKEINFO = makeinfo
MULTIPATH_NUM = 1
OSPF6D = 
OSPFD = 
OTHER_METHOD = 
PACKAGE = zebra
RANLIB = ranlib
RIPD = ripd
RIPNGD = 
RTREAD_METHOD = rtread_netlink.o
RT_METHOD = rt_netlink.o
VERSION = 0.92a
VTYSH = 
ZEBRA = zebra

INCLUDES =  -I.. -I$(top_srcdir) -I$(top_srcdir)/lib
DEFS = -DHAVE_CONFIG_H -DSYSCONFDIR=\"$(sysconfdir)/\" -DMULTIPATH_NUM=1
INSTALL_SDATA = /usr/bin/install -c -m 600

LIB_IPV6 = 

ipforward = ipforward_proc.o
if_method = if_netlink.o
if_proc = if_proc.o
rt_method = rt_netlink.o
rtread_method = rtread_netlink.o
kernel_method = 
other_method = 

otherobj = $(ipforward) $(if_method) $(if_proc) $(rt_method) 	$(rtread_method) $(kernel_method) $(other_method)


#sbin_PROGRAMS = zebra client
sbin_PROGRAMS = zebra

zebra_SOURCES =  	zserv.c main.c interface.c connected.c ioctl.c rib.c redistribute.c 	debug.c rtadv.c zebra_snmp.c


noinst_HEADERS =  	connected.h ioctl.h rib.h rt.h zserv.h redistribute.h debug.h rtadv.h 	interface.h ipforward.h irdp.h


zebra_LDADD = ../lib/libzebra.a $(otherobj)

#client_SOURCES = \
#	client_main.c

sysconf_DATA = zebra.conf.sample

EXTRA_DIST = $(sysconf_DATA) if_ioctl.c if_netlink.c if_proc.c if_sysctl.c 	ipforward_aix.c ipforward_ews.c ipforward_proc.c 	ipforward_solaris.c ipforward_sysctl.c rt_ioctl.c rt_netlink.c 	rt_socket.c rtread_netlink.c rtread_proc.c rtread_sysctl.c 	rtread_getmsg.c kernel_socket.c kernel_netlink.c mtu_kvm.c 	GNOME-SMI GNOME-PRODUCT-ZEBRA-MIB irdp.c

mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(sbin_PROGRAMS)

CPPFLAGS = 
LDFLAGS = 
LIBS = -lm -lcrypt 
zebra_OBJECTS =  zserv.o main.o interface.o connected.o ioctl.o rib.o \
redistribute.o debug.o rtadv.o zebra_snmp.o
zebra_DEPENDENCIES =  ../lib/libzebra.a
zebra_LDFLAGS = 
CFLAGS = -g -Os -Wall
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
DATA =  $(sysconf_DATA)

HEADERS =  $(noinst_HEADERS)

DIST_COMMON =  ChangeLog Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP_ENV = --best
SOURCES = $(zebra_SOURCES)
OBJECTS = $(zebra_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .o .s
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --foreign --include-deps zebra/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-sbinPROGRAMS:

clean-sbinPROGRAMS:
	-test -z "$(sbin_PROGRAMS)" || rm -f $(sbin_PROGRAMS)

distclean-sbinPROGRAMS:

maintainer-clean-sbinPROGRAMS:

install-sbinPROGRAMS: $(sbin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(sbindir)
	@list='$(sbin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(DESTDIR)$(sbindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	     $(INSTALL_PROGRAM) $$p $(DESTDIR)$(sbindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done

uninstall-sbinPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(sbin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(sbindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done

.c.o:
	$(COMPILE) -c $<

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

uninstall-sysconfDATA:
	@$(NORMAL_UNINSTALL)
	list='$(sysconf_DATA)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(sysconfdir)/$$p; \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = zebra

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am: install-sbinPROGRAMS install-sysconfDATA
install-exec: install-exec-am

install-data-am:
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am: uninstall-sbinPROGRAMS uninstall-sysconfDATA
uninstall: uninstall-am
all-am: Makefile $(PROGRAMS) $(DATA) $(HEADERS)
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(sbindir) $(DESTDIR)$(sysconfdir)


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-sbinPROGRAMS mostlyclean-compile \
		mostlyclean-tags mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-sbinPROGRAMS clean-compile clean-tags clean-generic \
		mostlyclean-am

clean: clean-am

distclean-am:  distclean-sbinPROGRAMS distclean-compile distclean-tags \
		distclean-generic clean-am

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-sbinPROGRAMS \
		maintainer-clean-compile maintainer-clean-tags \
		maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: mostlyclean-sbinPROGRAMS distclean-sbinPROGRAMS \
clean-sbinPROGRAMS maintainer-clean-sbinPROGRAMS uninstall-sbinPROGRAMS \
install-sbinPROGRAMS mostlyclean-compile distclean-compile \
clean-compile maintainer-clean-compile uninstall-sysconfDATA \
install-sysconfDATA tags mostlyclean-tags distclean-tags clean-tags \
maintainer-clean-tags distdir info-am info dvi-am dvi check check-am \
installcheck-am installcheck install-exec-am install-exec \
install-data-am install-data install-am install uninstall-am uninstall \
all-redirect all-am all installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


zebra : $(zebra_OBJECTS) $(otherobj) ../lib/libzebra.a
	$(CC) -g -o zebra $(zebra_OBJECTS) $(otherobj) ../lib/libzebra.a $(LIBS) $(LIB_IPV6)

client : client_main.o ../lib/libzebra.a
	$(CC) -g -o client client_main.o ../lib/libzebra.a $(LIBS) $(LIB_IPV6)

install-sysconfDATA: $(sysconf_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)
	@list='$(sysconf_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_SDATA) $(srcdir)/$$p $(DESTDIR)$(sysconfdir)/$$p"; \
	    $(INSTALL_SDATA) $(srcdir)/$$p $(DESTDIR)$(sysconfdir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_SDATA) $$p $(DESTDIR)$(sysconfdir)/$$p"; \
	    $(INSTALL_SDATA) $$p $(DESTDIR)$(sysconfdir)/$$p; \
	  fi; fi; \
	done

depend:
	@$(CPP) -MM $(INCLUDES) $(LDFLAGS) *.c

client_main.o: client_main.c ../lib/prefix.h ../lib/stream.h \
 ../lib/zclient.h ../lib/if.h ../lib/linklist.h ../lib/thread.h \
 ../lib/table.h ../zebra/rib.h ../zebra/zserv.h
connected.o: connected.c ../lib/prefix.h ../lib/linklist.h ../lib/if.h \
 ../lib/table.h rib.h ../lib/log.h ../zebra/zserv.h \
 ../zebra/redistribute.h
debug.o: debug.c ../lib/command.h ../lib/vector.h ../lib/vty.h debug.h
if_ioctl.o: if_ioctl.c ../lib/if.h ../lib/linklist.h \
 ../lib/sockunion.h ../lib/prefix.h ioctl.h connected.h \
 ../lib/memory.h ../lib/log.h ../zebra/interface.h
if_netlink.o: if_netlink.c
if_proc.o: if_proc.c ../lib/if.h ../lib/linklist.h ../lib/prefix.h \
 ../lib/log.h ../zebra/ioctl.h ../zebra/connected.h \
 ../zebra/interface.h
if_sysctl.o: if_sysctl.c ../lib/if.h ../lib/linklist.h \
 ../lib/sockunion.h ../lib/prefix.h connected.h ../lib/memory.h \
 ioctl.h ../lib/log.h
interface.o: interface.c ../lib/if.h ../lib/linklist.h ../lib/vty.h \
 ../lib/sockunion.h ../lib/prefix.h ../lib/command.h ../lib/vector.h \
 ../lib/memory.h ioctl.h connected.h ../lib/log.h ../lib/zclient.h \
 ../zebra/interface.h ../zebra/rtadv.h ../zebra/rib.h ../zebra/zserv.h \
 ../zebra/redistribute.h ../lib/table.h ../zebra/debug.h
ioctl.o: ioctl.c ../lib/linklist.h ../lib/if.h ../lib/prefix.h ioctl.h \
 ../lib/log.h ../zebra/rib.h ../zebra/rt.h
ipforward_aix.o: ipforward_aix.c
ipforward_ews.o: ipforward_ews.c
ipforward_proc.o: ipforward_proc.c
ipforward_solaris.o: ipforward_solaris.c ../lib/memory.h
ipforward_sysctl.o: ipforward_sysctl.c ../lib/log.h
irdp.o: irdp.c ../lib/if.h ../lib/linklist.h ../lib/stream.h \
 ../lib/memory.h ../lib/command.h ../lib/vector.h ../lib/vty.h \
 ../lib/log.h ../lib/sockunion.h ../zebra/irdp.h
kernel_netlink.o: kernel_netlink.c
kernel_socket.o: kernel_socket.c ../lib/if.h ../lib/linklist.h \
 ../lib/prefix.h ../lib/sockunion.h connected.h ../lib/memory.h \
 ioctl.h ../lib/log.h ../lib/str.h ../lib/table.h rib.h \
 ../zebra/interface.h ../zebra/zserv.h ../zebra/debug.h \
 ../lib/thread.h
main.o: main.c ../lib/version.h ../lib/command.h ../lib/vector.h \
 ../lib/vty.h ../lib/thread.h ../lib/filter.h ../lib/if.h \
 ../lib/linklist.h ../lib/memory.h ../lib/prefix.h ../lib/log.h \
 ../zebra/rib.h ../zebra/zserv.h ../zebra/debug.h
mtu_kvm.o: mtu_kvm.c ../lib/if.h ../lib/linklist.h
redistribute.o: redistribute.c ../lib/vector.h ../lib/vty.h \
 ../lib/command.h ../lib/prefix.h ../lib/table.h ../lib/stream.h \
 ../lib/zclient.h ../lib/if.h ../lib/linklist.h ../lib/log.h \
 ../zebra/rib.h ../zebra/zserv.h ../zebra/redistribute.h \
 ../zebra/debug.h
rib.o: rib.c ../lib/prefix.h ../lib/table.h ../lib/memory.h \
 ../lib/vty.h ../lib/str.h ../lib/command.h ../lib/vector.h \
 ../lib/linklist.h ../lib/if.h ../lib/log.h ../lib/sockunion.h \
 ../zebra/rib.h ../zebra/rt.h ../zebra/zserv.h ../zebra/redistribute.h \
 ../zebra/debug.h
rt_ioctl.o: rt_ioctl.c ../lib/prefix.h ../lib/log.h ../lib/if.h \
 ../lib/linklist.h ../zebra/rib.h ../zebra/debug.h
rt_netlink.o: rt_netlink.c ../lib/linklist.h ../lib/if.h ../lib/log.h \
 ../lib/prefix.h connected.h ../lib/table.h rib.h ../zebra/zserv.h \
 ../zebra/redistribute.h ../zebra/interface.h ../zebra/debug.h \
 ../lib/thread.h
rt_socket.o: rt_socket.c ../lib/if.h ../lib/linklist.h ../lib/prefix.h \
 ../lib/sockunion.h ../lib/log.h ../lib/str.h ../zebra/debug.h \
 ../zebra/rib.h
rtadv.o: rtadv.c ../lib/memory.h ../lib/sockopt.h ../lib/thread.h \
 ../lib/if.h ../lib/linklist.h ../lib/log.h ../lib/prefix.h \
 ../lib/command.h ../lib/vector.h ../lib/vty.h ../zebra/interface.h \
 ../zebra/rtadv.h ../zebra/debug.h
rtread_getmsg.o: rtread_getmsg.c ../lib/prefix.h ../lib/log.h \
 ../lib/if.h ../lib/linklist.h ../zebra/rib.h
rtread_netlink.o: rtread_netlink.c
rtread_proc.o: rtread_proc.c ../lib/prefix.h ../lib/log.h ../lib/if.h \
 ../lib/linklist.h rib.h
rtread_sysctl.o: rtread_sysctl.c ../lib/memory.h ../lib/log.h
zebra_snmp.o: zebra_snmp.c
zserv.o: zserv.c ../lib/prefix.h ../lib/command.h ../lib/vector.h \
 ../lib/vty.h ../lib/if.h ../lib/linklist.h ../lib/thread.h \
 ../lib/stream.h ../lib/memory.h ../lib/table.h rib.h ../lib/network.h \
 ../lib/sockunion.h ../lib/log.h ../lib/zclient.h ../zebra/zserv.h \
 ../zebra/redistribute.h ../zebra/debug.h ../zebra/ipforward.h

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
