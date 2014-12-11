#
# Makefile.am for snapper/pam
#

if HAVE_PAM

AM_CFLAGS = -D_GNU_SOURCE -Wwrite-strings

AM_CPPFLAGS = -I$(top_srcdir) $(DBUS_CFLAGS)

securelibdir = $(shell echo /`basename $(libdir)`/security)

securelib_LTLIBRARIES = pam_snapper.la

pam_snapper_la_LDFLAGS = -no-undefined -avoid-version -module
pam_snapper_la_LIBADD = -lpam $(DBUS_LIBS)

endif

