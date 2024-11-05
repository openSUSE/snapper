#
# Makefile.am for snapper/data
#

EXTRA_DIST = sysconfig.snapper base.txt lvm.txt x11.txt snapper.logrotate	\
	default-config org.opensuse.Snapper.conf org.opensuse.Snapper.service	\
	zypp-plugin.conf timeline.service timeline.timer cleanup.service	\
	cleanup.timer boot.service boot.timer backup.service backup.timer	\
	snapperd.service

install-data-local:
	install -D -m 644 snapper.logrotate $(DESTDIR)/etc/logrotate.d/snapper

	install -d -m 755 $(DESTDIR)/etc/snapper/configs
	install -d -m 755 $(DESTDIR)/etc/snapper/backup-configs
	install -d -m 755 $(DESTDIR)/etc/snapper/certs

	install -d -m 755 $(DESTDIR)/usr/lib/snapper/plugins

	install -d -m 755 $(DESTDIR)/usr/share/snapper/config-templates
	install -D -m 644 default-config $(DESTDIR)/usr/share/snapper/config-templates/default

	install -d -m 755 $(DESTDIR)/usr/share/snapper/filters
	install -D -m 644 base.txt $(DESTDIR)/usr/share/snapper/filters/base.txt
	install -D -m 644 lvm.txt $(DESTDIR)/usr/share/snapper/filters/lvm.txt
	install -D -m 644 x11.txt $(DESTDIR)/usr/share/snapper/filters/x11.txt

	install -D -m 644 org.opensuse.Snapper.conf $(DESTDIR)/usr/share/dbus-1/system.d/org.opensuse.Snapper.conf
	install -D -m 644 org.opensuse.Snapper.service $(DESTDIR)/usr/share/dbus-1/system-services/org.opensuse.Snapper.service

if ENABLE_SYSTEMD
	install -D -m 644 timeline.service $(DESTDIR)/usr/lib/systemd/system/snapper-timeline.service
	install -D -m 644 timeline.timer $(DESTDIR)/usr/lib/systemd/system/snapper-timeline.timer
	install -D -m 644 cleanup.service $(DESTDIR)/usr/lib/systemd/system/snapper-cleanup.service
	install -D -m 644 cleanup.timer $(DESTDIR)/usr/lib/systemd/system/snapper-cleanup.timer
	install -D -m 644 boot.service $(DESTDIR)/usr/lib/systemd/system/snapper-boot.service
	install -D -m 644 boot.timer $(DESTDIR)/usr/lib/systemd/system/snapper-boot.timer
	install -D -m 644 backup.service $(DESTDIR)/usr/lib/systemd/system/snapper-backup.service
	install -D -m 644 backup.timer $(DESTDIR)/usr/lib/systemd/system/snapper-backup.timer
	install -D -m 644 snapperd.service $(DESTDIR)/usr/lib/systemd/system/snapperd.service
endif

if HAVE_ZYPP
	install -D -m 644 zypp-plugin.conf $(DESTDIR)/usr/share/snapper/zypp-plugin.conf
endif
