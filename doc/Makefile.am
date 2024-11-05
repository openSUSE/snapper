#
# Makefile.am for snapper/doc
#

if ENABLE_DOC

man_MANS = snapper.8 snapperd.8 snapper-configs.5 snbk.8 snapper-backup-configs.5

if HAVE_PAM
man_MANS += pam_snapper.8
endif

if HAVE_ZYPP
man_MANS += snapper-zypp-plugin.conf.5 snapper-zypp-plugin.8
endif

if ENABLE_ROLLBACK
man_MANS += mksubvolume.8
endif

.xml.5: .xml
	$(XSLTPROC) --nonet manpages.xsl $<

.xml.8: .xml
	$(XSLTPROC) --nonet manpages.xsl $<

html: $(man_MANS:%.5=%.html) $(man_MANS:%.8=%.html)

.xml.html: .xml
	$(XSLTPROC) --nonet html.xsl $< > $@

endif

EXTRA_DIST = dbus-protocol.txt manpages.xsl html.xsl selinux-readme.txt

clean-local:
	rm -f *.{5,8} *.html

