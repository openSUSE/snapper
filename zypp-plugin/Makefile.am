SUBDIRS = testsuite

if HAVE_ZYPP

plugindir = /usr/lib/zypp/plugins/commit
plugin_PROGRAMS = snapper-zypp-plugin

AM_CPPFLAGS = $(DBUS_CFLAGS) $(XML2_CFLAGS)
AM_CXXFLAGS = -Wsuggest-override
snapper_zypp_plugin_SOURCES = \
    snapper_zypp_plugin.cc \
    zypp_commit_plugin.cc zypp_commit_plugin.h \
    zypp_plugin.cc zypp_plugin.h
snapper_zypp_plugin_LDFLAGS = \
    ../client/libclient.la \
    ../snapper/libsnapper.la \
    ../dbus/libdbus.la \
    -lboost_regex

endif
