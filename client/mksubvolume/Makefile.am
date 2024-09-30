#
# Makefile.am for snapper/client/mksubvolume
#

AM_CPPFLAGS = -I$(top_srcdir)

if ENABLE_BTRFS

if ENABLE_ROLLBACK

sbin_PROGRAMS = mksubvolume

mksubvolume_SOURCES =			\
	mksubvolume.cc

mksubvolume_LDADD =			\
	../../snapper/libsnapper.la 	\
	../utils/libutils.la

endif

endif
