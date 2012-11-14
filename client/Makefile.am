#
# Makefile.am for snapper/client
#

SUBDIRS = utils

INCLUDES = -I$(top_srcdir) $(DBUS_CFLAGS)


bin_PROGRAMS = snapper

snapper_SOURCES =			\
	snapper.cc			\
	types.cc	types.h		\
	commands.cc	commands.h	\
	cleanup.cc	cleanup.h

snapper_LDADD = ../snapper/libsnapper.la utils/libutils.la ../dbus/libdbus.la

