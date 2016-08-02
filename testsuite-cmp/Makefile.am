#
# Makefile.am for snapper/testsuite-cmp
#

AM_CPPFLAGS = -I$(top_srcdir)
AM_LDFLAGS = -lboost_system

LDADD = ../snapper/libsnapper.la

noinst_SCRIPTS = run-all

noinst_PROGRAMS = cmp

cmp_SOURCES = cmp.cc

EXTRA_DIST = $(noinst_SCRIPTS)

