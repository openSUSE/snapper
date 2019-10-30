#
# Makefile.am for snapper/client/Command/GetConfig
#

SUBDIRS = ConfigData

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libcommandgetconfig.la

libcommandgetconfig_la_SOURCES =	    \
	Options.cc	    Options.h	    \
	ConfigData.cc	    ConfigData.h

libcommandgetconfig_la_LIBADD =		\
	../../../snapper/libsnapper.la	\
	../../utils/libutils.la		\
	../../../dbus/libdbus.la	\
	ConfigData/libcommandgetconfigconfigdata.la
