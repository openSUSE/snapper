#
# Makefile.am for snapper/testsuite-real
#

CXXFLAGS += -std=gnu++0x

INCLUDES = -I$(top_srcdir)

LDADD = ../snapper/libsnapper.la

noinst_SCRIPTS = run-all

noinst_PROGRAMS = simple1 permissions1 permissions2 owner1 owner2

simple1_SOURCES = simple1.cc common.h common.cc

permissions1_SOURCES = permissions1.cc common.h common.cc
permissions2_SOURCES = permissions2.cc common.h common.cc

owner1_SOURCES = owner1.cc common.h common.cc
owner2_SOURCES = owner2.cc common.h common.cc

