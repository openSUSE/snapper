#
# Makefile.am for snapper/scripts
#

pam_snapperdir = /usr/lib/pam_snapper

pam_snapper_SCRIPTS =			\
	pam_snapper_homeconvert.sh	\
	pam_snapper_pamconfig.sh	\
	pam_snapper_useradd.sh		\
	pam_snapper_userdel.sh

EXTRA_DIST = snapper-hourly snapper-daily zypp-plugin.py $(pam_snapper_SCRIPTS)

install-data-local:
	install -D snapper-hourly $(DESTDIR)/etc/cron.hourly/suse.de-snapper
	install -D snapper-daily $(DESTDIR)/etc/cron.daily/suse.de-snapper
if HAVE_ZYPP
	install -D zypp-plugin.py $(DESTDIR)/usr/lib/zypp/plugins/commit/snapper.py
endif

