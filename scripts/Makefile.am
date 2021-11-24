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

EXTRA_DIST = snapper-hourly snapper-daily bash-completion.bash $(pam_snapper_SCRIPTS)

install-data-local:
	install -D snapper-hourly $(DESTDIR)/etc/cron.hourly/suse.de-snapper
	install -D snapper-daily $(DESTDIR)/etc/cron.daily/suse.de-snapper
	install -D --mode a+r,u+w bash-completion.bash $(DESTDIR)/usr/share/bash-completion/completions/snapper

