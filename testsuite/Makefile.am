#
# Makefile.am for snapper/testsuite
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

LDADD = ../snapper/libsnapper.la ../dbus/libdbus.la -lboost_unit_test_framework

check_PROGRAMS = sysconfig-get1.test dirname1.test basename1.test		\
	equal-date.test dbus-escape.test

TESTS = $(check_PROGRAMS)

AM_DEFAULT_SOURCE_EXT = .cc

EXTRA_DIST = $(noinst_SCRIPTS) sysconfig-get1.txt

equal_date_test_LDADD = -lboost_unit_test_framework ../client/utils/libutils.la

