#
# Makefile.am for snapper/doc
#

man_MANS = snapper.8 snapperd.8

if HAVE_PAM
man_MANS += pam_snapper.8
endif

.sgml.8: .sgml
	docbook2man $<

TXTS = dbus-protocol.txt

EXTRA_DIST = $(TXTS)

