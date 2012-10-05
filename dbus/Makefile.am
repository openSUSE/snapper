#
# Makefile.am for snapper/dbus
#

INCLUDES = -I$(top_srcdir) -I/usr/include/dbus-1.0 -I$(libdir)/dbus-1.0/include

noinst_LTLIBRARIES = libdbus.la

libdbus_la_SOURCES =					\
	DBusMessage.cc		DBusMessage.h		\
	DBusConnection.cc	DBusConnection.h	\
	DBusMainLoop.cc		DBusMainLoop.h

libdbus_la_LIBADD = -ldbus-1

