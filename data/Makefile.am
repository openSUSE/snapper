#
# Makefile.am for snapper/data
#

EXTRA_DIST = sysconfig.snapper base.txt lvm.txt x11.txt snapper.logrotate	\
	default-config org.opensuse.Snapper.conf org.opensuse.Snapper.service	\
	zypp-plugin.conf timeline.service timeline.timer cleanup.service	\
	cleanup.timer boot.service boot.timer

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

if ENABLE_SYSTEMD
	install -D -m 644 timeline.service $(DESTDIR)/usr/lib/systemd/system/snapper-timeline.service
	install -D -m 644 timeline.timer $(DESTDIR)/usr/lib/systemd/system/snapper-timeline.timer
	install -D -m 644 cleanup.service $(DESTDIR)/usr/lib/systemd/system/snapper-cleanup.service
	install -D -m 644 cleanup.timer $(DESTDIR)/usr/lib/systemd/system/snapper-cleanup.timer
	install -D -m 644 boot.service $(DESTDIR)/usr/lib/systemd/system/snapper-boot.service
	install -D -m 644 boot.timer $(DESTDIR)/usr/lib/systemd/system/snapper-boot.timer
endif

if HAVE_ZYPP
	install -D -m 644 zypp-plugin.conf $(DESTDIR)/etc/snapper/zypp-plugin.conf
endif
