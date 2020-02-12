---
title: Display of Used Space per Snapshot
author: Arvin Schnell
layout: post
---

Btrfs with quota enabled can already show the exclusive space used by
each snapshot using btrfs tools. When removing a snapshot the
exclusive space becomes free as explained in
[qgroups.pdf](http://sensille.com/qgroups.pdf).

Now snapper displays the exclusive space used by each snapshot if
quota is enabled on btrfs:

~~~
# snapper --iso list
Type   | # | Pre # | Date                | User | Used Space | Cleanup | Description           | Userdata     
-------+---+-------+---------------------+------+------------+---------+-----------------------+--------------
single | 0 |       |                     | root |            |         | current               |              
single | 1 |       | 2018-10-04 21:00:11 | root | 15.77 MiB  |         | first root filesystem |              
single | 2 |       | 2018-10-04 21:19:47 | root | 13.78 MiB  | number  | after installation    | important=yes
~~~

To display correct values for the used space a btrfs quota rescan is
performed. Since that needs some time the display of the used space
can be disabled with --disable-used-space.

Here is an example where the display works fine. First we create a big
file and then we create a new snapshot.

~~~
# dd if=/dev/urandom of=/big-data bs=1M count=1024
1024+0 records in
1024+0 records out
1073741824 bytes (1.1 GB, 1.0 GiB) copied, 12.245 s, 87.7 MB/s
# snapper create --cleanup number --print-number
3
~~~

Since the big file is not only part of snapshot #3 but also of the
root file system the data is not exclusive for snapshot #3 and thus
the used space is much small than 1 GiB.

~~~
# snapper --iso list
Type   | # | Pre # | Date                | User | Used Space | Cleanup | Description           | Userdata     
-------+---+-------+---------------------+------+------------+---------+-----------------------+--------------
single | 0 |       |                     | root |            |         | current               |              
single | 1 |       | 2018-10-04 21:00:11 | root | 16.00 KiB  |         | first root filesystem |              
single | 2 |       | 2018-10-04 21:19:47 | root | 13.78 MiB  | number  | after installation    | important=yes
single | 3 |       | 2018-10-04 21:32:53 | root | 16.00 KiB  | number  |                       |              
~~~

Only after removing the big file from the root file system the data is
exclusive to snapshot #3 and thus the used space grows to 1 GiB.

~~~
# rm /big-data
# snapper --iso list
# snapper --iso list
Type   | # | Pre # | Date                | User | Used Space | Cleanup | Description           | Userdata     
-------+---+-------+---------------------+------+------------+---------+-----------------------+--------------
single | 0 |       |                     | root |            |         | current               |              
single | 1 |       | 2018-10-04 21:00:11 | root | 80.00 KiB  |         | first root filesystem |              
single | 2 |       | 2018-10-04 21:19:47 | root | 13.78 MiB  | number  | after installation    | important=yes
single | 3 |       | 2018-10-04 21:32:53 | root | 1.00 GiB   | number  |                       |              
~~~

Removing snapshot #3 will make the 1 GiB available on the file system.

Unfortunately this does not work so well in other cases where pre- and
post-snapshot are created.

~~~
# snapper create --command "dd if=/dev/urandom of=/big-data bs=1M count=1024" --description "create big data" --cleanup number --print-number
1024+0 records in
1024+0 records out
1073741824 bytes (1.1 GB, 1.0 GiB) copied, 12.0915 s, 88.8 MB/s
4..5
# snapper --iso list
Type   | # | Pre # | Date                | User | Used Space | Cleanup | Description           | Userdata     
-------+---+-------+---------------------+------+------------+---------+-----------------------+--------------
[...]
pre    | 4 |       | 2018-10-04 21:34:08 | root | 80.00 KiB  | number  | create big data       |              
post   | 5 | 4     | 2018-10-04 21:34:20 | root | 16.00 KiB  | number  | create big data       |              
# snapper create --command "rm /big-data" --description "remove big data"  --cleanup number --print-number
6..7
# snapper --iso list
Type   | # | Pre # | Date                | User | Used Space | Cleanup | Description           | Userdata     
-------+---+-------+---------------------+------+------------+---------+-----------------------+--------------
[...]
pre    | 4 |       | 2018-10-04 21:34:08 | root | 80.00 KiB  | number  | create big data       |              
post   | 5 | 4     | 2018-10-04 21:34:20 | root | 16.00 KiB  | number  | create big data       |              
pre    | 6 |       | 2018-10-04 21:35:54 | root | 16.00 KiB  | number  | remove big data       |              
post   | 7 | 6     | 2018-10-04 21:35:54 | root | 16.00 KiB  | number  | remove big data       |              
~~~

Although the big file has been removed from the root file system the
used space does not show up in any of the snapshots. That is because
both snapshot #5 and #6 include the data thus it is not exclusive to
any of them. You would have to remove both snapshot #5 and #6 to make
the 1 GiB available on the file system.

For users familiar with btrfs qgroups follows an explanation how to
know in advance how much space is freed when removing several
snapshots. A new qgroup has to be created and all snapshots to be
removed have to be added to it. Then the exclusive space of the new
qgroup can be queried. First get the ID of the snapshots:

~~~
# btrfs subvolume list /
[...]
ID 276 gen 129 top level 267 path @/.snapshots/5/snapshot
ID 277 gen 133 top level 267 path @/.snapshots/6/snapshot
[...]
~~~

The qgroup ids are "0/276" and "0/277". Now create a new higher level
qgroup, add the snapshots, run quota rescan and then query the
exclusive space:

~~~
# btrfs qgroup create 1/1 /
# btrfs qgroup assign 0/276 1/1 /
WARNING: quotas may be inconsistent, rescan needed
# btrfs qgroup assign 0/277 1/1 /
WARNING: quotas may be inconsistent, rescan needed
# btrfs quota rescan -w /
# btrfs qgroup show -p /
qgroupid         rfer         excl parent  
--------         ----         ---- ------  
[...]
0/276         2.79GiB     16.00KiB 1/0,1/1 
0/277         2.79GiB     16.00KiB 1/0,1/1 
[...]
1/1           2.79GiB      1.00GiB ---     
~~~

So removing snapshot #5 and #6 will free 1 GiB as expected.

This feature is available in snapper since version 0.6.0.
