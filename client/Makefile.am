#
# Makefile.am for snapper/client
#

SUBDIRS = utils

INCLUDES = -I$(top_srcdir) -I/usr/include/dbus-1.0 -I$(libdir)/dbus-1.0/include


bin_PROGRAMS = snapper

snapper_SOURCES =			\
	snapper.cc			\
	types.cc	types.h		\
	commands.cc	commands.h	\
	cleanup.cc	cleanup.h

snapper_LDADD = ../snapper/libsnapper.la utils/libutils.la ../dbus/libdbus.la	\
	-ldbus-1 -lboost_thread-mt

