#
# Makefile.am for snapper
#

SUBDIRS = snapper examples dbus server client scripts pam data doc po		\
	testsuite testsuite-real testsuite-cmp

AUTOMAKE_OPTIONS = foreign dist-bzip2 no-dist-gzip
ACLOCAL_AMFLAGS = -Im4

doc_DATA = AUTHORS COPYING

EXTRA_DIST = $(doc_DATA) VERSION LIBVERSION

snapper-$(VERSION).tar.bz2: dist-bzip2

package: snapper-$(VERSION).tar.bz2
	rm -f package/snapper-*.tar.bz2
	mv snapper-$(VERSION).tar.bz2 package/

