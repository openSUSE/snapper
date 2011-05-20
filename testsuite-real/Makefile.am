#
# Makefile.am for snapper/testsuite-real
#

CXXFLAGS += -std=gnu++0x

INCLUDES = -I$(top_srcdir)

LDADD = ../snapper/libsnapper.la

noinst_SCRIPTS = run-all

noinst_PROGRAMS = simple1 permissions1 permissions2 owner1 owner2 owner3	\
	directory1 missing-directory1 error1 error2 error4

simple1_SOURCES = simple1.cc common.h common.cc

permissions1_SOURCES = permissions1.cc common.h common.cc
permissions2_SOURCES = permissions2.cc common.h common.cc

owner1_SOURCES = owner1.cc common.h common.cc
owner2_SOURCES = owner2.cc common.h common.cc
owner3_SOURCES = owner3.cc common.h common.cc

directory1_SOURCES = directory1.cc common.h common.cc

missing_directory1_SOURCES = missing-directory1.cc common.h common.cc

error1_SOURCES = error1.cc common.h common.cc
error2_SOURCES = error2.cc common.h common.cc
error4_SOURCES = error4.cc common.h common.cc

EXTRA_DIST = $(noinst_SCRIPTS)

