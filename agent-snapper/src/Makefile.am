
# Makefile.am for .../agent-snapper/src
#

AM_CXXFLAGS = -DY2LOG=\"agent-snapper\" -Wall

plugin_LTLIBRARIES = libpy2ag_snapper.la
noinst_LTLIBRARIES = liby2ag_snapper.la

liby2ag_snapper_la_SOURCES =				\
	SnapperAgent.cc					\
	SnapperAgent.h 
liby2ag_snapper_la_LDFLAGS = -version-info 2:0
liby2ag_snapper_la_LIBADD = @AGENT_LIBADD@ -lsnapper


libpy2ag_snapper_la_SOURCES =				\
        $(liby2ag_snapper_la_SOURCES)			\
        Y2CCSnapperAgent.cc
libpy2ag_snapper_la_LDFLAGS = -version-info 2:0
libpy2ag_snapper_la_LIBADD = @AGENT_LIBADD@ -lsnapper


INCLUDES = -I$(includedir)
