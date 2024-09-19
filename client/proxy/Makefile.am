#
# Makefile.am for snapper/client/proxy
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libclient.la libproxy.la

libclient_la_SOURCES =			\
	types.cc	types.h		\
	commands.cc	commands.h	\
	errors.cc	errors.h

libclient_la_LIBADD = 			\
	../utils/libutils.la		\
	../../dbus/libdbus.la

libproxy_la_SOURCES =			\
	proxy.cc	proxy.h		\
	proxy-dbus.cc	proxy-dbus.h	\
	proxy-lib.cc	proxy-lib.h
