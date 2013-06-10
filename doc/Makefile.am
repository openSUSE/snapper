#
# Makefile.am for snapper/doc
#

man_MANS = snapper.8 snapperd.8

if HAVE_PAM
man_MANS += pam_snapper.8
endif

.xml.8: .xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

TXTS = dbus-protocol.txt

EXTRA_DIST = $(TXTS)

html: $(man_MANS:%.8=%.html)

.xml.html: .xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl $< > $@

clean-local:
	rm -f *.8 manpage.* *.html

