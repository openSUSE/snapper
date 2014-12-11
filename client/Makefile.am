#
# Makefile.am for snapper/client
#

SUBDIRS = utils

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

bin_PROGRAMS = snapper

snapper_SOURCES =			\
	snapper.cc			\
	types.cc	types.h		\
	commands.cc	commands.h	\
	cleanup.cc	cleanup.h	\
	misc.cc		misc.h		\
	errors.cc	errors.h

snapper_LDADD = ../snapper/libsnapper.la utils/libutils.la ../dbus/libdbus.la

noinst_PROGRAMS = systemd-helper

systemd_helper_SOURCES =		\
	systemd-helper.cc		\
	types.cc	types.h		\
	commands.cc	commands.h	\
	cleanup.cc	cleanup.h	\
	errors.cc	errors.h

systemd_helper_LDADD = ../snapper/libsnapper.la utils/libutils.la ../dbus/libdbus.la

