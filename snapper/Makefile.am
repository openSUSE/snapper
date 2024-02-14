#
# Makefile.am for snapper/snapper
#

AM_CXXFLAGS = -D_FILE_OFFSET_BITS=64

lib_LTLIBRARIES = libsnapper.la

libsnapper_la_SOURCES =					\
	Snapper.cc		Snapper.h		\
	Snapshot.cc		Snapshot.h		\
	Comparison.cc		Comparison.h		\
	ComparisonImpl.cc	ComparisonImpl.h	\
	Filesystem.cc		Filesystem.h		\
	File.cc			File.h			\
	XmlFile.cc		XmlFile.h		\
	Enum.cc			Enum.h			\
	AppUtil.cc		AppUtil.h		\
	AppUtil2.cc					\
	FileUtils.cc		FileUtils.h		\
	XAttributes.cc		XAttributes.h		\
	Log.cc			Log.h			\
	Logger.cc		Logger.h		\
	Compare.cc		Compare.h		\
	SystemCmd.cc		SystemCmd.h		\
	AsciiFile.cc		AsciiFile.h		\
	Acls.cc			Acls.h			\
	Plugins.cc		Plugins.h		\
	PluginsImpl.cc		PluginsImpl.h		\
	Hooks.cc		Hooks.h			\
	Systemctl.cc		Systemctl.h		\
	Exception.cc		Exception.h		\
	SnapperTmpl.h					\
	SnapperTypes.h					\
	SnapperDefines.h				\
	Version.cc		Version.h

if ENABLE_BTRFS
libsnapper_la_SOURCES +=				\
	Btrfs.cc		Btrfs.h			\
	BtrfsUtils.cc		BtrfsUtils.h
endif

if ENABLE_BCACHEFS
libsnapper_la_SOURCES +=				\
	Bcachefs.cc		Bcachefs.h		\
	BcachefsUtils.cc	BcachefsUtils.h
endif

if ENABLE_EXT4
libsnapper_la_SOURCES +=				\
	Ext4.cc			Ext4.h
endif

if ENABLE_LVM
libsnapper_la_SOURCES +=				\
	Lvm.cc			Lvm.h			\
	LvmCache.cc		LvmCache.h		\
	LvmUtils.cc		LvmUtils.h
endif

if ENABLE_ROLLBACK
libsnapper_la_SOURCES +=				\
	MntTable.cc		MntTable.h
endif

if ENABLE_SELINUX
libsnapper_la_SOURCES +=				\
	Selinux.cc		Selinux.h
endif

libsnapper_la_CPPFLAGS = $(XML2_CFLAGS) $(ZLIB_CFLAGS)
libsnapper_la_LDFLAGS = -version-info @LIBVERSION_INFO@
libsnapper_la_LIBADD = -lboost_thread -lboost_system $(XML2_LIBS) -lacl $(ZLIB_LIBS)
if ENABLE_ROLLBACK
libsnapper_la_LIBADD += -lmount
endif
if ENABLE_BTRFS
libsnapper_la_CPPFLAGS += $(LIBBTRFSUTIL_CFLAGS)
libsnapper_la_LIBADD += $(LIBBTRFSUTIL_LIBS)
endif
if ENABLE_SELINUX
libsnapper_la_LIBADD += -lselinux
endif

pkgincludedir = $(includedir)/snapper

pkginclude_HEADERS =					\
	Version.h					\
	Snapper.h					\
	Snapshot.h					\
	Plugins.h					\
	File.h						\
	Comparison.h					\
	AsciiFile.h					\
	Exception.h					\
	Logger.h
