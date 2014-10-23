#
# Makefile.am for snapper/testsuite
#

INCLUDES = -I$(top_srcdir)

LDADD = ../snapper/libsnapper.la -lboost_unit_test_framework

check_PROGRAMS = sysconfig-get1 dirname1 basename1

TESTS = $(check_PROGRAMS)

AM_DEFAULT_SOURCE_EXT = .cc

EXTRA_DIST = $(noinst_SCRIPTS) sysconfig-get1.txt

