---
title: Snapper and LVM thin-provisioned Snapshots
author: Arvin Schnell
layout: post
---

SUSE's Hackweek 8 allowed me to implement support for LVM
thin-provisioned snapshots in snapper. Since thin-provisioned
snapshots themself are new I will shortly show their usage.

Unfortunately openSUSE 12.2 RC1 does not include LVM tools with
thin-provisioning so you have to compile them on your own. First
install the
[thin-provisioning-tools](https://github.com/jthornber/thin-provisioning-tools). Then
install [LVM](ftp://sources.redhat.com/pub/lvm2/LVM2.2.02.96.tgz) with
thin-provisioning enabled (configure option –with-thin=internal).

To setup LVM we first have to create a volume group either using the
LVM tools or YaST. I assume it’s named test. Then we create a storage
pool with 3 GB space.

~~~
# modprobe dm-thin-pool
# lvcreate --thin test/pool --size 3G
~~~

Now we can create a thin-provisioned logical volume named thin with a
size of 5 GB. The size can be larger than the pool since data is only
allocated from the pool when needed.

~~~
# lvcreate --thin test/pool --virtualsize 5G --name thin

# mkfs.ext4 /dev/test/thin
# mkdir /thin
# mount /dev/test/thin /thin
~~~

Finally we can create a snapshot from the logical volume.

~~~
# lvcreate --snapshot --name thin-snap1 /dev/test/thin

# mkdir /thin-snapshot
# mount /dev/test/thin-snap1 /thin-snapshot
~~~

Space for the snapshot is also allocated from the pool when
needed. The command lvs gives an overview of the allocated space.

~~~
# lvs
LV         VG   Attr     LSize Pool Origin Data%  Move Log Copy%  Convert
pool       test twi-a-tz 3.00g               4.24
thin       test Vwi-aotz 5.00g pool          2.54
thin-snap1 test Vwi-a-tz 5.00g pool thin     2.54
~~~

After installing [snapper version
0.0.12](https://build.opensuse.org/project/show/filesystems:snapper)
or later we can create a config for the logical volume thin.

~~~
# snapper -c thin create-config --fstype="lvm(ext4)" /thin
~~~

As a simple test we can create a new file and see that snapper detects its creation.

~~~
# snapper -c thin create --command "touch /thin/lenny"

# snapper -c thin list
Type   | # | Pre # | Date                          | Cleanup | Description | Userdata
-------+---+-------+-------------------------------+---------+-------------+---------
single | 0 |       |                               |         | current     |
pre    | 1 |       | Tue 24 Jul 2012 15:49:51 CEST |         |             |
post   | 2 | 1     | Tue 24 Jul 2012 15:49:51 CEST |         |             |

# snapper -c thin status 1..2
+... /thin/lenny
~~~

So now you can use snapper even if you don’t trust btrfs. Feedback is welcomed.

_Originally posted on
[lizards.opensuse.org](https://lizards.opensuse.org/2012/07/25/snapper-lvm/)._
