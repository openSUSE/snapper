#
# Makefile.am for snapper/dbus
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libdbus.la

libdbus_la_SOURCES =					\
	DBusMessage.cc		DBusMessage.h		\
	DBusPipe.cc		DBusPipe.h		\
	DBusConnection.cc	DBusConnection.h	\
	DBusMainLoop.cc		DBusMainLoop.h

libdbus_la_LIBADD = $(DBUS_LIBS)
libdbus_la_LDFLAGS = -lboost_system -lboost_thread
