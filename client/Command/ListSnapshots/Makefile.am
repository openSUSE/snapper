#
# Makefile.am for snapper/client/Command/ListSnapshots
#

SUBDIRS = SnappersData

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libcommandlistsnapshots.la

libcommandlistsnapshots_la_SOURCES =		\
	Options.cc		Options.h	\
	SnappersData.cc		SnappersData.h

libcommandlistsnapshots_la_LIBADD =	\
	../../../snapper/libsnapper.la	\
	../../utils/libutils.la		\
	../../../dbus/libdbus.la	\
	SnappersData/libcommandlistsnapshotssnappersdata.la
