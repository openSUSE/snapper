#
# Makefile.am for snapper/data
#

EXTRA_DIST = sysconfig.snapper base.txt x11.txt snapper.logrotate default-config	\
	org.opensuse.snapper.conf org.opensuse.snapper.service

install-data-local:
	install -D -m 644 snapper.logrotate $(DESTDIR)/etc/logrotate.d/snapper
	install -d -m 755 $(DESTDIR)/etc/snapper/configs
	install -D -m 644 default-config $(DESTDIR)/etc/snapper/config-templates/default
	install -d -m 755 $(DESTDIR)/etc/snapper/filters
	install -D -m 644 base.txt $(DESTDIR)/etc/snapper/filters/base.txt
	install -D -m 644 x11.txt $(DESTDIR)/etc/snapper/filters/x11.txt
	install -D -m 644 org.opensuse.snapper.conf $(DESTDIR)/etc/dbus-1/system.d/org.opensuse.snapper.conf
	install -D -m 644 org.opensuse.snapper.service $(DESTDIR)/usr/share/dbus-1/system-services/org.opensuse.snapper.service
