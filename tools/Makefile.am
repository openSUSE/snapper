#
# Makefile.am for snapper/tools
#

SUBDIRS = utils

INCLUDES = -I$(top_srcdir)


toolsbindir = /usr/lib/snapper/bin

toolsbin_PROGRAMS = compare-dirs

compare_dirs_SOURCES = compare-dirs.cc

compare_dirs_LDADD = ../snapper/libsnapper.la


bin_PROGRAMS = snapper

snapper_SOURCES = snapper.cc

snapper_LDADD = ../snapper/libsnapper.la utils/libutils.la

