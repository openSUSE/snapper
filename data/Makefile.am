#
# Makefile.am for snapper/data
#

fillup_DATA = sysconfig.snapper

EXTRA_DIST = $(fillup_DATA) snapper.logrotate

install-data-local:
	install -D -m 644 snapper.logrotate $(DESTDIR)/etc/logrotate.d/snapper
	install -d -m 755 $(DESTDIR)/etc/snapper/configs

