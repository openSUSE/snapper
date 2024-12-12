#
# Makefile.am for snapper/client/installation-helper
#

AM_CPPFLAGS = -I$(top_srcdir) $(XML2_CFLAGS)

libexecdir = /usr/lib/snapper

if ENABLE_BTRFS

libexec_PROGRAMS = installation-helper

installation_helper_SOURCES =		\
	installation-helper.cc		\
	../misc.cc	../misc.h

installation_helper_LDADD =		\
	../../snapper/libsnapper.la	\
	../utils/libutils.la

endif
