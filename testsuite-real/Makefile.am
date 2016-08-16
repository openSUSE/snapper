#
# Makefile.am for snapper/testsuite-real
#

CXXFLAGS += -std=gnu++0x

AM_CPPFLAGS = -I$(top_srcdir)

LDADD = ../snapper/libsnapper.la

if HAVE_XATTRS
TMP_XATST = xattrs1 xattrs2 xattrs3 xattrs4
endif

noinst_SCRIPTS = run-all

noinst_PROGRAMS = simple1 permissions1 permissions2 permissions3 owner1 owner2 \
	owner3 directory1 missing-directory1 error1 error2 error4 \
	$(TMP_XATST) test-btrfsutils ug-tests

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

xattrs1_SOURCES = xattrs1.cc xattrs_utils.cc xattrs_utils.h common.h common.cc
xattrs2_SOURCES = xattrs2.cc xattrs_utils.cc xattrs_utils.h common.h common.cc
xattrs3_SOURCES = xattrs3.cc xattrs_utils.cc xattrs_utils.h common.h common.cc
xattrs4_SOURCES = xattrs4.cc xattrs_utils.cc xattrs_utils.h common.h common.cc

test_btrfsutils_SOURCES = test-btrfsutils.cc

ug_tests_SOURCES = ug-tests.cc

EXTRA_DIST = $(noinst_SCRIPTS)

