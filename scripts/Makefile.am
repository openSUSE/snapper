#
# Makefile.am for snapper/scripts
#

EXTRA_DIST = snapper-hourly snapper-daily zypp-plugin.py

install-data-local:
	install -D snapper-hourly $(DESTDIR)/etc/cron.hourly/suse.de-snapper
	install -D snapper-daily $(DESTDIR)/etc/cron.daily/suse.de-snapper
	install -D zypp-plugin.py $(DESTDIR)/usr/lib/zypp/plugins/commit/snapper.py

