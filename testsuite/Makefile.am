#
# Makefile.am for snapper/testsuite
#

CXXFLAGS += -std=gnu++0x

INCLUDES = -I$(top_srcdir)

LDADD = ../snapper/libsnapper.la

noinst_SCRIPTS = run-all

noinst_PROGRAMS = sysconfig-get1 dirname1 basename1

sysconfig_get1_SOURCES = sysconfig-get1.cc common.h

dirname1_SOURCES = dirname1.cc common.h
basename1_SOURCES = basename1.cc common.h

EXTRA_DIST = $(noinst_SCRIPTS) sysconfig-get1.txt

check:
	./run-all

