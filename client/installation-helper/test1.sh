#!/usr/bin/bash -x

umount /test/.snapshots
umount /test

mkfs.btrfs -f /dev/sdc1

mount /dev/sdc1 /test

/usr/lib/snapper/installation-helper --root-prefix /test --step filesystem

umount /test

mount /dev/sdc1 /test
mount /dev/sdc1 -o subvol=.snapshots /test/.snapshots

/usr/lib/snapper/installation-helper --root-prefix /test --step config --description initial --userdata a=1

snapper --no-dbus --root /test ls

