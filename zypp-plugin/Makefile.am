SUBDIRS = testsuite

if HAVE_ZYPP

plugindir = /usr/lib/zypp/plugins/commit
plugin_PROGRAMS = snapper-zypp-plugin

AM_CPPFLAGS = $(DBUS_CFLAGS) $(XML2_CFLAGS) $(JSONC_CFLAGS)
AM_CXXFLAGS = -Wsuggest-override
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

check_PROGRAMS = regex.test
TESTS = $(check_PROGRAMS)
regex_test_SOURCES = regex_test.cc \
    solvable_matcher.cc solvable_matcher.h
regex_test_LDADD = \
    ../snapper/libsnapper.la \
    -lboost_unit_test_framework

endif
