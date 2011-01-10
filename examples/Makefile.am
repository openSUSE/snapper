#
# Makefile.am for libsnapper/examples
#

INCLUDES = -I$(top_srcdir)

LDADD = ../snapper/libsnapper.la

noinst_PROGRAMS = List Create

List_SOURCES = List.cc

Create_SOURCES = Create.cc

exampledir = $(docdir)/examples

example_DATA = List.cc Create.cc

EXTRA_DIST = $(example_DATA) Makefile.example

install-data-local:
	/usr/bin/install -d -p -m 755 $(DESTDIR)$(exampledir)
	/usr/bin/install -p -m 644 Makefile.example $(DESTDIR)$(exampledir)/Makefile
