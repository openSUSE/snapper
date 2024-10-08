#
# Makefile.am for snapper/client/utils
#

AM_CPPFLAGS = -I$(top_srcdir)

noinst_LTLIBRARIES = libutils.la

libutils_la_SOURCES =					\
	Table.cc	    	Table.h			\
	text.cc		    	text.h			\
	help.cc		    	help.h			\
	console.cc	    	console.h		\
	equal-date.cc	    	equal-date.h		\
	GetOpts.cc	    	GetOpts.h		\
	Range.cc	    	Range.h			\
	HumanString.cc	    	HumanString.h		\
	Limit.cc	    	Limit.h			\
	TableFormatter.cc   	TableFormatter.h	\
	CsvFormatter.cc	    	CsvFormatter.h		\
	JsonFormatter.cc    	JsonFormatter.h		\
	OutputOptions.h

libutils_la_LIBADD = ../../snapper/libsnapper.la -ltinfo
