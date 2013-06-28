#
# Makefile.am for snapper/doc
#

man_MANS = snapper.8 snapperd.8

if HAVE_PAM
man_MANS += pam_snapper.8
endif

.xml.8: .xml
	$(XSLTPROC) --nonet manpages.xsl $<

html: $(man_MANS:%.8=%.html)

.xml.html: .xml
	$(XSLTPROC) --nonet html.xsl $< > $@

EXTRA_DIST = dbus-protocol.txt manpages.xsl html.xsl

clean-local:
	rm -f *.8 *.html

