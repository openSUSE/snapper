#
# Makefile.am for snapper/testsuite-cmp
#

AM_CPPFLAGS = -I$(top_srcdir)

LDADD = ../snapper/libsnapper.la

noinst_SCRIPTS = run-all

noinst_PROGRAMS = cmp

cmp_SOURCES = cmp.cc
cmp_LDADD = $(LDADD) -lboost_system

EXTRA_DIST = $(noinst_SCRIPTS)

