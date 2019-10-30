#
# Makefile.am for snapper/client/Command/ListConfigs/SnappersData
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libcommandlistconfigssnappersdata.la

libcommandlistconfigssnappersdata_la_SOURCES =	\
	Table.cc	Table.h			\
	Csv.cc		Csv.h			\
	Json.cc		Json.h

libcommandlistconfigssnappersdata_la_LIBADD =	\
	../../../../snapper/libsnapper.la	\
	../../../utils/libutils.la		\
	../../../../dbus/libdbus.la
