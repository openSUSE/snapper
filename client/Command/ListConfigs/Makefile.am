#
# Makefile.am for snapper/client/Command/ListConfigs
#

SUBDIRS = SnappersData

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libcommandlistconfigs.la

libcommandlistconfigs_la_SOURCES =		\
	Options.cc		Options.h	\
	SnappersData.cc		SnappersData.h

libcommandlistconfigs_la_LIBADD =	\
	../../../snapper/libsnapper.la	\
	../../utils/libutils.la		\
	../../../dbus/libdbus.la	\
	SnappersData/libcommandlistconfigssnappersdata.la
