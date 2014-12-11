#
# Makefile.am for libsnapper/bindings/python
#

EXTRA_DIST = libsnapper.i

AM_CXXFLAGS = -fPIC -DPIC

PYLIBVER ?= python$(PYTHON_VERSION)

AM_CPPFLAGS = -I. -I${top_srcdir} -I/usr/include/$(PYLIBVER)

DEPENDENCIES =						\
	${top_srcdir}/snapper/Factory.h			\
	${top_srcdir}/snapper/Snapper.h			\
	${top_srcdir}/snapper/Snapshot.h		\
	${top_srcdir}/snapper/File.h			\
	${top_srcdir}/snapper/Comparison.h		\
	${top_srcdir}/snapper/Exception.h

LIBS = $(top_builddir)/snapper/libsnapper.la

python_PYTHON = libsnapper.py

pyexec_LTLIBRARIES = _libsnapper.la

_libsnapper_la_LDFLAGS = -module -avoid-version
_libsnapper_la_HEADERS: $(top_builddir)/config.h

_libsnapper_la_DEPENDENCIES =				\
	$(DEPENDENCIES)					\
	${top_builddir}/snapper/libsnapper.la

nodist__libsnapper_la_SOURCES = libsnapper_wrap.cxx

libsnapper.py libsnapper_wrap.cxx: libsnapper.i $(DEPENDENCIES)
	swig -o libsnapper_wrap.cxx -c++ -python ${AM_CPPFLAGS} libsnapper.i

CLEANFILES = libsnapper.py libsnapper_wrap.cxx
