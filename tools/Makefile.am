#
# Makefile.am for snapper/tools
#

INCLUDES = -I$(top_srcdir)


toolsbindir = /usr/lib/snapper/bin

toolsbin_PROGRAMS = compare-dirs

compare_dirs_SOURCES = compare-dirs.cc
compare_dirs_LDADD = ../snapper/libsnapper.la -lboost_thread

