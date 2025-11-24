#
# Makefile.am for snapper/stomp/testsuite
#

SUBDIRS = .

LDADD =								\
	../libstomp.la						\
	-lboost_unit_test_framework

check_PROGRAMS =						\
	read1.test write1.test escape.test strip.test

AM_DEFAULT_SOURCE_EXT = .cc

TESTS = $(check_PROGRAMS)

