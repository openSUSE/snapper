#
# Makefile.am for snapper/client/snbk
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS) $(JSON_C_CFLAGS)

sbin_PROGRAMS = snbk

snbk_SOURCES = 						\
	snbk.cc						\
	cmd.h						\
	cmd-list-configs.cc				\
	cmd-list.cc					\
	cmd-transfer.cc					\
	cmd-restore.cc					\
	cmd-delete.cc					\
	cmd-transfer-and-delete.cc			\
	BackupConfig.cc		BackupConfig.h		\
	TheBigThing.cc		TheBigThing.h		\
	GlobalOptions.cc	GlobalOptions.h		\
	Shell.cc		Shell.h			\
	CmdBtrfs.cc		CmdBtrfs.h		\
	CmdLs.cc		CmdLs.h			\
	JsonFile.cc		JsonFile.h

snbk_LDADD =				\
	../../snapper/libsnapper.la	\
	../../dbus/libdbus.la		\
	../proxy/libproxy.la		\
	../proxy/libclient.la		\
        $(JSON_C_LIBS)
