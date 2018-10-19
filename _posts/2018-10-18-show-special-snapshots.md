---
title: Display of Special Snapshots
author: Arvin Schnell
layout: post
piwik: true
---

For btrfs there are some special snapshots: Once the snapshot
currently mounted and the snapshot that will be mounted next time
unless a specific snapshot is selected in grub.

Now snapper informs the user about these special snapshot in the
output of "snapper list": A "-" after the snapshot number indicates
that the snapshot is the currently mounted snapshot and a "+"
indicates that the snapshot will be mounted next time. If both
conditions apply a "*" is displayed.

Following is a tour to see how the special snapshot change during a
rollback.

Right after installation snapshot #1 is both the currently mounted and
the next to be mounted snapshot:

~~~
# snapper --iso list --disable-used-space
 # | Type   | Pre # | Date                | User | Cleanup | Description           | Userdata     
---+--------+-------+---------------------+------+---------+-----------------------+--------------
0  | single |       |                     | root |         | current               |              
1* | single |       | 2018-10-18 10:33:50 | root |         | first root filesystem |              
2  | single |       | 2018-10-18 10:43:45 | root | number  | after installation    | important=yes
~~~

Now we can ruin our system. The special snapshots do not change here:

~~~
# snapper create --command "rm /boot/initrd*" --description "ruin system" --print-number
3..4
# snapper --iso ls --disable-used-space
 # | Type   | Pre # | Date                | User | Cleanup | Description           | Userdata     
---+--------+-------+---------------------+------+---------+-----------------------+--------------
0  | single |       |                     | root |         | current               |              
1* | single |       | 2018-10-18 10:33:50 | root |         | first root filesystem |              
2  | single |       | 2018-10-18 10:43:45 | root | number  | after installation    | important=yes
3  | pre    |       | 2018-10-18 11:03:11 | root |         | ruin system           |              
4  | post   |     3 | 2018-10-18 11:03:11 | root |         | ruin system           |              
~~~

Reboot now without further actions will show that the system is indeed
ruined. In order to boot correctly we can select to boot from snapshot
#3 in grub. Now the special snapshot have changed:

~~~
# snapper --iso ls --disable-used-space
 # | Type   | Pre # | Date                | User | Cleanup | Description           | Userdata     
---+--------+-------+---------------------+------+---------+-----------------------+--------------
0  | single |       |                     | root |         | current               |              
1+ | single |       | 2018-10-18 10:33:50 | root |         | first root filesystem |              
2  | single |       | 2018-10-18 10:43:45 | root | number  | after installation    | important=yes
3- | pre    |       | 2018-10-18 11:03:11 | root |         | ruin system           |              
4  | post   |     3 | 2018-10-18 11:03:11 | root |         | ruin system           |              
~~~

Snapshot #3 is now mounted. Snapshot #1 is still the snapshot that
would be mounted without interaction in grub. Using "snapper rollback"
will now create new snapshots and mark one of them as the to be
mounted snapshot:

~~~
# snapper rollback
Creating read-only snapshot of default subvolume. (Snapshot 5.)
Creating read-write snapshot of current subvolume. (Snapshot 6.)
Setting default subvolume to snapshot 6.
# snapper --iso ls --disable-used-space
 # | Type   | Pre # | Date                | User | Cleanup | Description           | Userdata     
---+--------+-------+---------------------+------+---------+-----------------------+--------------
0  | single |       |                     | root |         | current               |              
1  | single |       | 2018-10-18 10:33:50 | root | number  | first root filesystem |              
2  | single |       | 2018-10-18 10:43:45 | root | number  | after installation    | important=yes
3- | pre    |       | 2018-10-18 11:03:11 | root |         | ruin system           |              
4  | post   |     3 | 2018-10-18 11:03:11 | root |         | ruin system           |              
5  | single |       | 2018-10-18 11:07:13 | root | number  | rollback backup of #1 | important=yes
6+ | single |       | 2018-10-18 11:07:13 | root |         |                       |              
~~~

Finally after a further reboot snapshot #6 is the mounted and to be
mounted next snapshot:

~~~
# snapper --iso ls --disable-used-space
 # | Type   | Pre # | Date                | User | Cleanup | Description           | Userdata     
---+--------+-------+---------------------+------+---------+-----------------------+--------------
0  | single |       |                     | root |         | current               |              
1  | single |       | 2018-10-18 10:33:50 | root | number  | first root filesystem |              
2  | single |       | 2018-10-18 10:43:45 | root | number  | after installation    | important=yes
3  | pre    |       | 2018-10-18 11:03:11 | root |         | ruin system           |              
4  | post   |     3 | 2018-10-18 11:03:11 | root |         | ruin system           |              
5  | single |       | 2018-10-18 11:07:13 | root | number  | rollback backup of #1 | important=yes
6* | single |       | 2018-10-18 11:07:13 | root |         |                       |              
~~~

Also it was never possible to delete these special snapshots. Now snapper
also blocks the deletion.

This feature is available in snapper since version 0.7.0.
