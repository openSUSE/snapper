#
# Makefile.am for snapper/scripts
#

if HAVE_PAM

pam_snapperdir = /usr/lib/pam_snapper

pam_snapper_SCRIPTS =			\
	pam_snapper_homeconvert.sh	\
	pam_snapper_pamconfig.sh	\
	pam_snapper_useradd.sh		\
	pam_snapper_userdel.sh

endif

EXTRA_DIST = snapper-create snapper-cleanup zypp-plugin.py $(pam_snapper_SCRIPTS)

install-data-local:
	install -D snapper-create $(DESTDIR)/etc/cron.hourly/suse.de-snapper-create
	install -D snapper-cleanup $(DESTDIR)/etc/cron.daily/suse.de-snapper-cleanup
if HAVE_ZYPP
	install -D zypp-plugin.py $(DESTDIR)/usr/lib/zypp/plugins/commit/snapper.py
endif

