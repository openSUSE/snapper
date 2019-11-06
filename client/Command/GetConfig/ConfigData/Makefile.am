#
# Makefile.am for snapper/client/Command/GetConfig/ConfigData
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libcommandgetconfigconfigdata.la

libcommandgetconfigconfigdata_la_SOURCES =	\
	Table.cc	Table.h			\
	Csv.cc		Csv.h			\
	Json.cc		Json.h

libcommandgetconfigconfigdata_la_LIBADD =	\
	../../../../snapper/libsnapper.la	\
	../../../utils/libutils.la		\
	../../../../dbus/libdbus.la
