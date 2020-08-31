#
# Makefile.am for snapper/testsuite
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

LDADD = ../snapper/libsnapper.la ../dbus/libdbus.la -lboost_unit_test_framework

check_PROGRAMS = sysconfig-get1.test dirname1.test basename1.test		    \
	equal-date.test dbus-escape.test cmp-lt.test humanstring.test table.test    \
	csv-formatter.test json-formatter.test getopts.test

if ENABLE_BTRFS_QUOTA
check_PROGRAMS += qgroup1.test
endif

TESTS = $(check_PROGRAMS)

AM_DEFAULT_SOURCE_EXT = .cc

EXTRA_DIST = $(noinst_SCRIPTS) sysconfig-get1.txt sysconfig-set1.txt

equal_date_test_LDADD = -lboost_unit_test_framework ../client/utils/libutils.la

humanstring_test_LDADD = -lboost_unit_test_framework ../client/utils/libutils.la

table_test_LDADD = -lboost_unit_test_framework ../client/utils/libutils.la

csv_formatter_test_LDADD = -lboost_unit_test_framework ../client/utils/libutils.la

json_formatter_test_LDADD = -lboost_unit_test_framework ../client/utils/libutils.la

getopts_test_LDADD = -lboost_unit_test_framework ../client/utils/libutils.la
