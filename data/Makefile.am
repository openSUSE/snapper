#
# Makefile.am for snapper/data
#

fillup_DATA = sysconfig.snapper

EXTRA_DIST = $(fillup_DATA) base.txt x11.txt snapper.logrotate example-config

install-data-local:
	install -D -m 644 snapper.logrotate $(DESTDIR)/etc/logrotate.d/snapper
	install -d -m 755 $(DESTDIR)/etc/snapper/configs
	install -D -m 644 default-config $(DESTDIR)/etc/snapper/config-templates/default
	install -d -m 755 $(DESTDIR)/etc/snapper/filters
	install -D -m 644 base.txt $(DESTDIR)/etc/snapper/filters/base.txt
	install -D -m 644 x11.txt $(DESTDIR)/etc/snapper/filters/x11.txt

