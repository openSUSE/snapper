#
# Makefile.am for snapper/tools/client
#

INCLUDES = -I$(top_srcdir) -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include


bin_PROGRAMS = snapper

snapper_SOURCES = snapper.cc types.cc commands.cc
snapper_LDADD = ../snapper/libsnapper.la ../tools/utils/libutils.la ../dbus/libdbus.la -ldbus-1

