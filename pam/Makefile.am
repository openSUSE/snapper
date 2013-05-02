#
# Makefile.am for snapper/pam
#

INCLUDES = -I$(top_srcdir) $(DBUS_CFLAGS)

securelibdir = $(shell echo /`basename $(libdir)`/security)

securelib_LTLIBRARIES = pam_snapper.la

pam_snapper_la_LDFLAGS = -no-undefined -avoid-version -module
pam_snapper_la_LIBADD = -lpam $(DBUS_LIBS)

