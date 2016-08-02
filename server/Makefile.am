#
# Makefile.am for snapper/server
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

sbin_PROGRAMS = snapperd

snapperd_SOURCES =					\
	snapperd.cc					\
	Client.cc		Client.h		\
	MetaSnapper.cc		MetaSnapper.h		\
	Background.cc		Background.h		\
	Types.cc		Types.h

snapperd_LDADD = ../snapper/libsnapper.la ../dbus/libdbus.la -lrt	\
	-lboost_thread -lboost_system
