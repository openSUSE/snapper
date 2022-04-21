#
# Makefile.am for snapper/testsuite-real
#

CXXFLAGS += -std=gnu++11

AM_CPPFLAGS = -I$(top_srcdir)

LDADD = ../snapper/libsnapper.la

testdir = $(libdir)/snapper/testsuite

test_DATA = CAUTION
test_SCRIPTS = run-all setup-and-run-all

test_PROGRAMS = simple1 permissions1 permissions2 permissions3 owner1 owner2	\
	owner3 directory1 missing-directory1 error1 error2 error4 ug-tests	\
	ascii-file

if ENABLE_BTRFS
test_PROGRAMS += test-btrfsutils
endif

if HAVE_XATTRS
test_PROGRAMS += xattrs1 xattrs2 xattrs3 xattrs4
endif

simple1_SOURCES = simple1.cc common.h common.cc

permissions1_SOURCES = permissions1.cc common.h common.cc
permissions2_SOURCES = permissions2.cc common.h common.cc
permissions3_SOURCES = permissions3.cc common.h common.cc

owner1_SOURCES = owner1.cc common.h common.cc
owner2_SOURCES = owner2.cc common.h common.cc
owner3_SOURCES = owner3.cc common.h common.cc

directory1_SOURCES = directory1.cc common.h common.cc

missing_directory1_SOURCES = missing-directory1.cc common.h common.cc

error1_SOURCES = error1.cc common.h common.cc
error2_SOURCES = error2.cc common.h common.cc
error4_SOURCES = error4.cc common.h common.cc

xattrs1_SOURCES = xattrs1.cc xattrs-utils.cc xattrs-utils.h common.h common.cc
xattrs2_SOURCES = xattrs2.cc xattrs-utils.cc xattrs-utils.h common.h common.cc
xattrs3_SOURCES = xattrs3.cc xattrs-utils.cc xattrs-utils.h common.h common.cc
xattrs4_SOURCES = xattrs4.cc xattrs-utils.cc xattrs-utils.h common.h common.cc

test_btrfsutils_SOURCES = test-btrfsutils.cc

ug_tests_SOURCES = ug-tests.cc

ascii_file_SOURCES = ascii-file.cc

EXTRA_DIST = $(test_DATA) $(test_SCRIPTS)

