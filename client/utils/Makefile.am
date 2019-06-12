#
# Makefile.am for snapper/client/utils
#

AM_CPPFLAGS = -I$(top_srcdir)

noinst_LTLIBRARIES = libutils.la

libutils_la_SOURCES =			\
	Table.cc	Table.h		\
	text.cc		text.h		\
	console.cc	console.h	\
	equal-date.cc	equal-date.h	\
	GetOpts.cc	GetOpts.h	\
	Range.cc	Range.h		\
	HumanString.cc	HumanString.h

libutils_la_LIBADD = ../../snapper/libsnapper.la

