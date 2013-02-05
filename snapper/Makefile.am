#
# Makefile.am for snapper/snapper
#

AM_CXXFLAGS = -D_FILE_OFFSET_BITS=64

INCLUDES = -I/usr/include/libxml2

lib_LTLIBRARIES = libsnapper.la

if HAVE_XATTRS
TMP_XA = XAttributes.cc	Xattributes.h
endif HAVE_XATTRS

libsnapper_la_SOURCES =					\
	Factory.cc		Factory.h		\
	Snapper.cc		Snapper.h		\
	Snapshot.cc		Snapshot.h		\
	Comparison.cc		Comparison.h		\
	Filesystem.cc		Filesystem.h		\
	File.cc			File.h			\
	XmlFile.cc		XmlFile.h		\
	Enum.cc			Enum.h			\
	AppUtil.cc		AppUtil.h		\
	FileUtils.cc		FileUtils.h		\
	Log.cc			Log.h			\
	Logger.cc		Logger.h		\
	Compare.cc		Compare.h		\
	SystemCmd.cc		SystemCmd.h		\
	AsciiFile.cc		AsciiFile.h		\
	Regex.cc		Regex.h			\
	Exception.h					\
	SnapperTmpl.h					\
	SnapperTypes.h					\
	SnapperDefines.h				\
	$(TMP_XA)



if ENABLE_BTRFS
libsnapper_la_SOURCES +=				\
	Btrfs.cc		Btrfs.h
endif

if ENABLE_EXT4
libsnapper_la_SOURCES +=				\
	Ext4.cc			Ext4.h
endif

if ENABLE_LVM
libsnapper_la_SOURCES +=				\
	Lvm.cc			Lvm.h
endif


libsnapper_la_LDFLAGS = -version-info @LIBVERSION_INFO@
libsnapper_la_LIBADD = -lboost_thread-mt -lxml2 -lz -lm

pkgincludedir = $(includedir)/snapper

pkginclude_HEADERS =					\
	Factory.h					\
	Snapper.h					\
	Snapshot.h					\
	File.h						\
	Comparison.h					\
	Exception.h					\
	Logger.h
