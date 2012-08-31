#
# Makefile.am for snapper/server
#

INCLUDES = -I$(top_srcdir) -I/usr/include/dbus-1.0 -I$(libdir)/dbus-1.0/include


sbin_PROGRAMS = snapperd

snapperd_SOURCES =					\
	snapperd.cc					\
	Client.cc		Client.h		\
	MetaSnapper.cc		MetaSnapper.h		\
	Types.cc		Types.h

snapperd_LDADD = ../snapper/libsnapper.la ../dbus/libdbus.la	\
	-ldbus-1 -lboost_thread-mt -lrt

