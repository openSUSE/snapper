#
# Makefile.am for snapper/client/Command/ListSnapshots/SnappersData
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libcommandlistsnapshotssnappersdata.la

libcommandlistsnapshotssnappersdata_la_SOURCES =	\
	Table.cc	Table.h				\
	Csv.cc		Csv.h				\
	Json.cc		Json.h

libcommandlistsnapshotssnappersdata_la_LIBADD =		\
	../../../../snapper/libsnapper.la		\
	../../../utils/libutils.la			\
	../../../../dbus/libdbus.la
