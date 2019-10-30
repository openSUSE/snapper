#
# Makefile.am for snapper/client/Command
#

SUBDIRS =		\
	ListSnapshots	\
	ListConfigs	\
	GetConfig

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

noinst_LTLIBRARIES = libcommand.la

libcommand_la_SOURCES =				\
	ListSnapshots.cc	ListSnapshots.h	\
	ListConfigs.cc		ListConfigs.h	\
	GetConfig.cc		GetConfig.h	\
	ColumnsOption.cc	ColumnsOption.h

libcommand_la_LIBADD =					\
	../../snapper/libsnapper.la			\
	../utils/libutils.la				\
	../../dbus/libdbus.la				\
	ListSnapshots/libcommandlistsnapshots.la	\
	ListConfigs/libcommandlistconfigs.la		\
	GetConfig/libcommandgetconfig.la
