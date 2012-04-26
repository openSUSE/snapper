#
# Makefile.am for snapper/tools/server
#

INCLUDES = -I$(top_srcdir)  -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include


sbin_PROGRAMS = snapperd

snapperd_SOURCES = snapperd.cc Client.cc Job.cc MetaSnapper.cc Types.cc
snapperd_LDADD = ../snapper/libsnapper.la ../dbus/libdbus.la -ldbus-1 -lboost_thread

