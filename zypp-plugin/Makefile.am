#
# Makefile.am for snapper/zypp-plugin
#

SUBDIRS = . testsuite

if HAVE_ZYPP

plugindir = /usr/lib/zypp/plugins/commit
plugin_PROGRAMS = snapper-zypp-plugin

AM_CPPFLAGS = $(DBUS_CFLAGS) $(XML2_CFLAGS) $(JSONC_CFLAGS)

snapper_zypp_plugin_SOURCES = \
	snapper-zypp-plugin.cc \
	solvable-matcher.cc solvable-matcher.h \
	zypp-commit-plugin.cc zypp-commit-plugin.h \
	zypp-plugin.cc zypp-plugin.h

snapper_zypp_plugin_LDADD = \
	../client/libclient.la \
	../snapper/libsnapper.la \
	../dbus/libdbus.la \
	$(JSONC_LIBS)

check_PROGRAMS = solvable-matcher.test forwarding-zypp-plugin

forwarding_zypp_plugin_SOURCES = \
	forwarding-zypp-plugin.cc \
	zypp-plugin.cc zypp-plugin.h

forwarding_zypp_plugin_LDADD = \
	../snapper/libsnapper.la \
	-lboost_system \
	-lpthread

TESTS = solvable-matcher.test

solvable_matcher_test_SOURCES = \
	solvable-matcher-test.cc \
	solvable-matcher.cc solvable-matcher.h

solvable_matcher_test_LDADD = \
	../snapper/libsnapper.la \
	$(XML2_LIBS) \
	-lboost_unit_test_framework

endif
