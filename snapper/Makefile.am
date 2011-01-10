#
# Makefile.am for libsnapper/snapper
#

lib_LTLIBRARIES = libsnapper.la

libsnapper_la_SOURCES =					\
	SnapperInterface.h				\
	Snapper.cc		Snapper.h		\
	XmlFile.cc		XmlFile.h		\
	Enum.cc			Enum.h			\
	AppUtil.cc		AppUtil.h		\
	SnapperTmpl.h

libsnapper_la_LDFLAGS = -version-info 2:0
libsnapper_la_LIBADD = -lblocxx -lxml2

pkgincludedir = $(includedir)/snapper

pkginclude_HEADERS =					\
	SnapperInterface.h

