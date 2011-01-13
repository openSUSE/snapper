#
# Makefile.am for snapper/snapper
#

lib_LTLIBRARIES = libsnapper.la

AM_CXXFLAGS = -D_FILE_OFFSET_BITS=64

libsnapper_la_SOURCES =					\
	SnapperInterface.h				\
	Snapper.cc		Snapper.h		\
	XmlFile.cc		XmlFile.h		\
	Enum.cc			Enum.h			\
	AppUtil.cc		AppUtil.h		\
	Files.cc		Files.h			\
	SystemCmd.cc		SystemCmd.h		\
	SnapperTmpl.h		SnapperDefines.h

libsnapper_la_LDFLAGS = -version-info 2:0
libsnapper_la_LIBADD = -lblocxx -lxml2

pkgincludedir = $(includedir)/snapper

pkginclude_HEADERS =					\
	SnapperInterface.h

