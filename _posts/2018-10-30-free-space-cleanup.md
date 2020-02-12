---
title: Cleanup based on Free Space in Filesystem
author: Arvin Schnell
layout: post
---

So far the space aware cleanup algorithms looked at the space the
snapshots are using. The algorithms are now extended to also look at
the free space of the filesystem. Per default snapshots are deleted
until at least 20% of the filesystem are free within the allowed
limits (e.g. the NUMBER_LIMIT variable).

Following is a tour to see how this works in practice.

We have a new installed openSUSE Tumbleweed system where we have
installed a few additional packages with zypper.

~~~
# snapper --iso list --disable-used-space
 # | Type   | Pre # | Date                | User | Cleanup | Description           | Userdata     
---+--------+-------+---------------------+------+---------+-----------------------+--------------
0  | single |       |                     | root |         | current               |              
1* | single |       | 2018-10-30 09:27:33 | root |         | first root filesystem |              
2  | single |       | 2018-10-30 09:38:58 | root | number  | after installation    | important=yes
3  | pre    |       | 2018-10-30 09:46:14 | root | number  | zypp(zypper)          | important=no 
4  | post   |     3 | 2018-10-30 09:46:27 | root | number  |                       | important=no 
5  | pre    |       | 2018-10-30 09:46:35 | root | number  | zypp(zypper)          | important=no 
6  | post   |     5 | 2018-10-30 09:46:55 | root | number  |                       | important=no 
7  | pre    |       | 2018-10-30 09:47:19 | root | number  | zypp(zypper)          | important=no 
8  | post   |     7 | 2018-10-30 09:47:36 | root | number  |                       | important=no 
~~~

~~~
# df -h /
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda2        14G  3.4G  8.8G  28% /
# btrfs qgroup show /
qgroupid         rfer         excl 
--------         ----         ---- 
[...]
1/0           2.98GiB    233.53MiB 
~~~

As you can see there is still plenty of free space available and the
btrfs quota group of the snapshots (1/0) uses only very little
exclusive space.

We set the limits to allow deleting of all unimportant snapshots.

~~~
snapper set-config NUMBER_LIMIT=0-10
~~~

Running the snapper number cleanup algorithm now does not delete any
snapshots since the snapshots need less than 50% of the space and the
free space is above 20%.

Now we fill the filesystem so that less than 20% is free.

~~~
# dd if=/dev/urandom of=/tmp/blob bs=1M count=8000
8000+0 records in
8000+0 records out
8388608000 bytes (8.4 GB, 7.8 GiB) copied, 56.2498 s, 149 MB/s
# df -h /
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda2        14G   12G 1005M  92% /
~~~

Running the snapper number cleanup algorithm now will delete snapshots
since the free space of the filesystem is below the limit of 20%.

~~~
# snapper cleanup number
# snapper --iso list --disable-used-space
 # | Type   | Pre # | Date                | User | Cleanup | Description           | Userdata     
---+--------+-------+---------------------+------+---------+-----------------------+--------------
0  | single |       |                     | root |         | current               |              
1* | single |       | 2018-10-30 09:27:33 | root |         | first root filesystem |              
2  | single |       | 2018-10-30 09:38:58 | root | number  | after installation    | important=yes
~~~

As you can see all snapshots snapper is allowed to delete have been
deleted. Unfortunately in this case this did not help much to make
free space available in the filesystem since most of the used space
was not used by snapshots in the first place.

~~~
# df -h /
Filesystem      Size  Used Avail Use% Mounted on
/dev/sda2        14G   12G  1.2G  91% /
~~~

This example already shows that this has consequences the
administrator has to be aware of. Any user can place files in /tmp
that will cause snapper to delete snapshots that so far where entirely
under control of the administrator.

The free space limit can be configured with the new FREE_LIMIT variable.

This feature is available in snapper since version 0.8.0.
