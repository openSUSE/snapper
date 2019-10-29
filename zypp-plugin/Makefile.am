plugindir = /usr/lib/zypp/plugins/commit
plugin_PROGRAMS = snapper-zypp-plugin

AM_CPPFLAGS = $(DBUS_CFLAGS)
AM_CXXFLAGS = -Wsuggest-override
snapper_zypp_plugin_SOURCES = snapper_zypp_plugin.cc
snapper_zypp_plugin_LDFLAGS = ../snapper/libsnapper.la ../dbus/libdbus.la -lboost_regex
