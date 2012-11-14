#
# Makefile.am for snapper/dbus
#

INCLUDES = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libdbus.la

libdbus_la_SOURCES =					\
	DBusMessage.cc		DBusMessage.h		\
	DBusConnection.cc	DBusConnection.h	\
	DBusMainLoop.cc		DBusMainLoop.h

libdbus_la_LIBADD =  $(DBUS_LIBS)

