#
# Makefile.am for snapper/dbus/testsuite
#

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

LDADD = ../../snapper/libsnapper.la ../libdbus.la -lboost_unit_test_framework

check_PROGRAMS = escape.test

TESTS = $(check_PROGRAMS)

AM_DEFAULT_SOURCE_EXT = .cc

