#
# Makefile.am for snapper
#

SUBDIRS = snapper examples tools scripts data po testsuite-real

AUTOMAKE_OPTIONS = foreign dist-bzip2 no-dist-gzip

doc_DATA = AUTHORS COPYING

EXTRA_DIST = $(doc_DATA) VERSION LIBVERSION


snapper-$(VERSION).tar.bz2: dist-bzip2

package-local: snapper-$(VERSION).tar.bz2
	rm -f package/snapper-*.tar.bz2
	rm -f package/*~
	rm -f package/*.bak
	mv snapper-$(VERSION).tar.bz2 package/

package: package-local

