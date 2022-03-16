#
# Makefile.am for snapper/zypp-plugin
#

SUBDIRS = . testsuite

if HAVE_ZYPP

plugindir = /usr/lib/zypp/plugins/commit
plugin_PROGRAMS = snapper-zypp-plugin

AM_CPPFLAGS = $(DBUS_CFLAGS) $(XML2_CFLAGS) $(JSONC_CFLAGS)

snapper_zypp_plugin_SOURCES = \
	snapper_zypp_plugin.cc \
	solvable_matcher.cc solvable_matcher.h \
	zypp_commit_plugin.cc zypp_commit_plugin.h \
	zypp_plugin.cc zypp_plugin.h

snapper_zypp_plugin_LDADD = \
	../client/libclient.la \
	../snapper/libsnapper.la \
	../dbus/libdbus.la \
	$(JSONC_LIBS)

check_PROGRAMS = solvable_matcher.test forwarding-zypp-plugin

forwarding_zypp_plugin_SOURCES = \
	forwarding_zypp_plugin.cc \
	zypp_plugin.cc zypp_plugin.h

forwarding_zypp_plugin_LDADD = \
	../snapper/libsnapper.la \
	-lboost_system \
	-lpthread

TESTS = solvable_matcher.test

solvable_matcher_test_SOURCES = \
	solvable_matcher_test.cc \
	solvable_matcher.cc solvable_matcher.h

solvable_matcher_test_LDADD = \
	../snapper/libsnapper.la \
	$(XML2_LIBS) \
	-lboost_unit_test_framework

endif
