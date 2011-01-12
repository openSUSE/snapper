#
# Makefile.am for snapper/examples
#

INCLUDES = -I$(top_srcdir)

LDADD = ../snapper/libsnapper.la

noinst_PROGRAMS = List Create CmpDirs

List_SOURCES = List.cc

Create_SOURCES = Create.cc

CmpDirs_SOURCES = CmpDirs.cc

exampledir = $(docdir)/examples

example_DATA = List.cc Create.cc CmpDirs.cc

EXTRA_DIST = $(example_DATA) Makefile.example

install-data-local:
	/usr/bin/install -d -p -m 755 $(DESTDIR)$(exampledir)
	/usr/bin/install -p -m 644 Makefile.example $(DESTDIR)$(exampledir)/Makefile
