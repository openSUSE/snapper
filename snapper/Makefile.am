#
# Makefile.am for snapper/snapper
#

AM_CXXFLAGS = -D_FILE_OFFSET_BITS=64

lib_LTLIBRARIES = libsnapper.la

libsnapper_la_SOURCES =					\
	Factory.cc		Factory.h		\
	Snapper.cc		Snapper.h		\
	Snapshot.cc		Snapshot.h		\
	Comparison.cc		Comparison.h		\
	File.cc			File.h			\
	XmlFile.cc		XmlFile.h		\
	Enum.cc			Enum.h			\
	AppUtil.cc		AppUtil.h		\
	Compare.cc		Compare.h		\
	SystemCmd.cc		SystemCmd.h		\
	AsciiFile.cc		AsciiFile.h		\
	Regex.cc		Regex.h			\
	SnapperTmpl.h					\
	SnapperTypes.h					\
	SnapperDefines.h

libsnapper_la_LDFLAGS = -version-info 2:0
libsnapper_la_LIBADD = -lblocxx -lxml2

pkgincludedir = $(includedir)/snapper

pkginclude_HEADERS =					\
	Factory.h					\
	Snapper.h					\
	Snapshot.h					\
	Comparison.h					\
	File.h

