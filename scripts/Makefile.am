#
# Makefile.am for snapper/scripts
#

EXTRA_DIST = snapper-hourly snapper-daily

install-data-local:
	install -D snapper-hourly $(DESTDIR)/etc/cron.hourly/suse.de-snapper
	install -D snapper-daily $(DESTDIR)/etc/cron.daily/suse.de-snapper

