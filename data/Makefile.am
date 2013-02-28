#
# Makefile.am for snapper/data
#

EXTRA_DIST = sysconfig.snapper base.txt lvm.txt x11.txt snapper.logrotate	\
	default-config org.opensuse.Snapper.conf org.opensuse.Snapper.service

install-data-local:
	install -D -m 644 snapper.logrotate $(DESTDIR)/etc/logrotate.d/snapper
	install -d -m 755 $(DESTDIR)/etc/snapper/configs
	install -D -m 644 default-config $(DESTDIR)/etc/snapper/config-templates/default
	install -d -m 755 $(DESTDIR)/etc/snapper/filters
	install -D -m 644 base.txt $(DESTDIR)/etc/snapper/filters/base.txt
	install -D -m 644 lvm.txt $(DESTDIR)/etc/snapper/filters/lvm.txt
	install -D -m 644 x11.txt $(DESTDIR)/etc/snapper/filters/x11.txt
	install -D -m 644 org.opensuse.Snapper.conf $(DESTDIR)/etc/dbus-1/system.d/org.opensuse.Snapper.conf
	install -D -m 644 org.opensuse.Snapper.service $(DESTDIR)/usr/share/dbus-1/system-services/org.opensuse.Snapper.service
