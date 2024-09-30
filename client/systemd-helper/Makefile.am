#
# Makefile.am for snapper/client/systemd-helper
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

libexecdir = /usr/lib/snapper

libexec_PROGRAMS = systemd-helper

systemd_helper_SOURCES =		\
	systemd-helper.cc		\
	../cleanup.cc	../cleanup.h	\
	../misc.cc	../misc.h

systemd_helper_LDADD = 			\
	../proxy/libproxy.la 		\
	../proxy/libclient.la 		\
	../../snapper/libsnapper.la	\
	../utils/libutils.la		\
	../../dbus/libdbus.la
