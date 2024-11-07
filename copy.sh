#!/bin/bash

HOST=192.168.1.91

rsync -v -ar --files-from=- / root@$HOST:/ <<EOF
/usr/bin/snapper
/usr/sbin/snapperd
/usr/sbin/snbk
/usr/lib64/libsnapper.so.7
/usr/lib64/libsnapper.so.7.4.3
/usr/lib/zypp/plugins/commit/snapper-zypp-plugin
/usr/lib/systemd/system/snapper-boot.service
/usr/lib/systemd/system/snapper-cleanup.service
/usr/lib/systemd/system/snapper-timeline.service
/usr/lib/systemd/system/snapper-backup.service
/usr/lib/systemd/system/snapper-backup.timer
/usr/lib/systemd/system/snapperd.service
/usr/lib/snapper/systemd-helper
EOF
