---
title: Automatic Cleanup of Snapshots created by Rollback
author: Arvin Schnell
layout: post
piwik: true
---

So far the user had to ensure that snapshots created by rollbacks get deleted
to avoid filling up the storage. This process has now been automated. During a
rollback snapper sets the cleanup algorithm to "number" for the snapshot
corresponding to the previous default subvolume and for the backup snapshot of
the previous default subvolume.

Here is a simple example. We start with a newly installed system. Snapshot #1
is the system and btrfs default subvolume. Snapshot #2 is a snapshot created
by YaST right after the installation:

~~~
# snapper --iso list
Type   | # | Pre # | Date                | User | Cleanup | Description           | Userdata
-------+---+-------+---------------------+------+---------+-----------------------+--------------
single | 0 |       |                     | root |         | current               |
single | 1 |       | 2017-05-09 10:05:13 | root |         | first root filesystem |
single | 2 |       | 2017-05-09 10:10:42 | root | number  | after installation    | important=yes

# btrfs subvolume get-default /
ID 259 gen 244 top level 258 path @/.snapshots/1/snapshot
~~~

Now we do a rollback:

~~~
# snapper rollback
Creating read-only snapshot of default subvolume. (Snapshot 3.)
Creating read-write snapshot of current subvolume. (Snapshot 4.)
Setting default subvolume to snapshot 4.
~~~

After the rollback the previous system (snapshot #1) and the backup snapshot
of the previous system (snapshot #3) have "number" set as cleanup algorithm:

~~~
# snapper --iso list
Type   | # | Pre # | Date                | User | Cleanup | Description           | Userdata
-------+---+-------+---------------------+------+---------+-----------------------+--------------
single | 0 |       |                     | root |         | current               |
single | 1 |       | 2017-05-09 10:05:13 | root | number  | first root filesystem |
single | 2 |       | 2017-05-09 10:10:42 | root | number  | after installation    | important=yes
single | 3 |       | 2017-05-09 12:17:30 | root | number  | rollback backup of #1 | important=yes
single | 4 |       | 2017-05-09 12:17:30 | root |         |                       |
~~~

As you can see the snapshots #1 and #3 have the cleanup algorithm set thus the
cleanup algorithm will automatically remove them as soon as the cleanup limits
are reached. Snapshot #3 also has the important flag set so it should be keep
longer in case the user wants to copy data from it. Snapshot #4 is the new
default btrfs subvolume and thus the system after a reboot.

This feature is available in snapper since version 0.5.0 and only for btrfs.
