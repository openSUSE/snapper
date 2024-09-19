#
# Makefile.am for snapper/client
#

SUBDIRS = utils

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libclient.la libproxy.la

libclient_la_SOURCES =			\
	types.cc	types.h		\
	commands.cc	commands.h	\
	errors.cc	errors.h

libclient_la_LIBADD = 			\
	utils/libutils.la		\
	../dbus/libdbus.la

libproxy_la_SOURCES =			\
	proxy.cc	proxy.h		\
	proxy-dbus.cc	proxy-dbus.h	\
	proxy-lib.cc	proxy-lib.h

bin_PROGRAMS = snapper

snapper_SOURCES =					\
	snapper.cc					\
	cmd.h						\
	cmd-list-configs.cc				\
	cmd-create-config.cc				\
	cmd-delete-config.cc				\
	cmd-get-config.cc				\
	cmd-set-config.cc				\
	cmd-list.cc					\
	cmd-create.cc					\
	cmd-modify.cc					\
	cmd-delete.cc					\
	cmd-mount.cc					\
	cmd-umount.cc					\
	cmd-status.cc					\
	cmd-diff.cc					\
	cmd-xadiff.cc					\
	cmd-undochange.cc				\
	cmd-rollback.cc					\
	cmd-setup-quota.cc				\
	cmd-cleanup.cc					\
	cmd-debug.cc					\
	cleanup.cc		cleanup.h		\
	locker.cc		locker.h		\
	misc.cc			misc.h			\
	MyFiles.cc		MyFiles.h		\
	GlobalOptions.cc	GlobalOptions.h

snapper_LDADD = 			\
	libproxy.la			\
	libclient.la			\
	../snapper/libsnapper.la	\
	utils/libutils.la		\
	../dbus/libdbus.la		\
	$(JSON_C_LIBS)

libexecdir = /usr/lib/snapper

libexec_PROGRAMS = systemd-helper

systemd_helper_SOURCES =		\
	systemd-helper.cc		\
	cleanup.cc	cleanup.h	\
	locker.cc	locker.h	\
	misc.cc		misc.h

systemd_helper_LDADD = 			\
	libproxy.la 			\
	libclient.la 			\
	../snapper/libsnapper.la	\
	utils/libutils.la		\
	../dbus/libdbus.la

if ENABLE_BTRFS

libexec_PROGRAMS += installation-helper

installation_helper_SOURCES =		\
	installation-helper.cc		\
	misc.cc		misc.h

installation_helper_LDADD = ../snapper/libsnapper.la utils/libutils.la

if ENABLE_ROLLBACK

sbin_PROGRAMS = mksubvolume

mksubvolume_SOURCES =			\
	mksubvolume.cc

mksubvolume_LDADD = ../snapper/libsnapper.la utils/libutils.la

endif

endif
