#
# Makefile.am for snapper/snapper
#

AM_CXXFLAGS = -D_FILE_OFFSET_BITS=64

lib_LTLIBRARIES = libsnapper.la

libsnapper_la_SOURCES =					\
	Snapper.cc		Snapper.h		\
	Factory.cc		Factory.h		\
	Snapshot.cc		Snapshot.h		\
	File.cc			File.h			\
	XmlFile.cc		XmlFile.h		\
	Enum.cc			Enum.h			\
	AppUtil.cc		AppUtil.h		\
	Compare.cc		Compare.h		\
	SystemCmd.cc		SystemCmd.h		\
	SnapperTmpl.h					\
	SnapperDefines.h

libsnapper_la_LDFLAGS = -version-info 2:0
libsnapper_la_LIBADD = -lblocxx -lxml2

pkgincludedir = $(includedir)/snapper

pkginclude_HEADERS =					\
	Snapper.h					\
	Factory.h					\
	Snapshot.h					\
	File.h

