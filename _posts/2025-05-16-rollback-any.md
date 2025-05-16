---
title: Rollback for other Subvolumes than Root
author: Arvin Schnell
layout: post
---

So far snapper only supports rollback for the root subvolume. The
reason is that rollback works by setting the btrfs default subvolume
and that obviously only exists once per btrfs file system.

But occasionally user wish to rollback also other subvolumes, e.g.
/home. Recently I was made aware of the possibility to rollback by
renaming snapshots instead of setting the default subvolume.

I have now come up with a setup that works with snapper. The following
is matched for openSUSE Tumbleweed and the /home subvolume (in the
usual @ subvolume, btrfs on /dev/sda2 and snapper already configured
for /home).

First we create a snapshot "0" that is mounted during system
startup. In /etc/fstab we use:

~~~
UUID=[...]  /home  btrfs  subvol=/@/home/.snapshots/0/snapshot  0 0
~~~

If we want to rollback to e.g. snapshot 10 we basically 1. rename
snapshot 0 to the next free number (e.g. 11) and set it read-only,
2. create a read-write snapshot of 10 as 0 and 3. reboot. That's it.

Although all of this is not yet implemented in snapper you can try
this workflow now. A simply Python program for doing the rollback is
provided at
https://github.com/openSUSE/snapper/blob/master/scripts/rollback-home.

Creating the initial setup is a bit tricky since it requires an
intermediate step:

~~~
umount /home
mount /dev/sda2 /mnt -o subvolid=5
mv /mnt/@/home /mnt/@/home-tmp
btrfs subvolume create /mnt/@/home
mv /mnt/@/home-tmp/.snapshots/ /mnt/@/home/
mkdir /mnt/@/home/.snapshots/0
mv /mnt/@/home-tmp /mnt/@/home/.snapshots/0/snapshot
mkdir /mnt/@/home/.snapshots/0/snapshot/.snapshots
~~~

Edit /etc/fstab:

~~~
UUID=[...]  /home  btrfs  subvol=/@/home/.snapshots/0/snapshot  0 0
UUID=[...]  /home/.snapshots  btrfs  subvol=/@/home/.snapshots  0 0
~~~

Then mount /home and /home/.snapshots or reboot.

Once disadvantage of the rename instead of setting the default
subvolume is that for a short period the snapshot 0 does not
exists. So in case of failures the system could be left in a broken
state. Long-term using renameat2 with RENAME_EXCHANGE should solve
that.

Feedback is appreciate.
