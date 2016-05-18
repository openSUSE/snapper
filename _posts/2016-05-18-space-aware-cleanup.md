---
title: Cleanup based on Disk Usage
author: Arvin Schnell
layout: post
---

Users have long requested the possibility to cleanup snapshot based on disk
usage. Finally the btrfs quota support looks mature enough for usage in
snapper so we were able to implement that desire in snapper.

You should become familiar with the concept of btrfs quota groups,
esp. "exclusive referenced space" and "qgroup hierarchy". You may read
http://sensille.com/qgroups.pdf, here I just give a short summary:

Each btrfs subvolume has an associated qgroup that tracks the amount of disk
space exclusive referenced by that subvolume. That means a 1 GiB file in that
subvolume will add 1 GiB to its exclusive referenced space if and only if that
file is not included in any other subvolume. As a consequence deleting the
subvolume will free the 1 GiB.

Unfortunately that information is not enough for snapper since most snapshots
will reference the same files so the exclusive space of each qgroup is usually
very small.

Fortunately btrfs offers a qgroup hierarchy. We can create a parent qgroup
that holds the qgroups of all snapshots. Now the exclusive space of this
parent qgroup tells us how much space all snapshots use. Snapper will watch
this value and try to keep it below a user-defined limit.

When you install your openSUSE system with YaST the space aware cleanup will
be enabled for the root filesystem. Here are the required steps for manually
setup:

~~~
# snapper setup-quota
~~~

This will enable btrfs quota on the root filesystem and create the btrfs
parent qgroup. We can query the id of the parent qgroup:

~~~
# snapper get-config
Key    | Value
-------+-------
QGROUP | 1/0
[...]
~~~

Then we allow the number cleanup algorithm to delete all except of the last
two number snapshots and the last four important number snapshots to satisfy
the space limit:

~~~
# snapper set-config NUMBER_LIMIT=2-10 NUMBER_LIMIT_IMPORTANT=4-10
~~~

Finally we run the cleanup algorithm:

~~~
# snapper cleanup number
~~~

This will ensure that all snapshot qgroups with a cleanup algorithm are
assigned to the parent qgroup and then delete snapshots to try to satisfy the
space limit.

You can view the btrfs qgroups:

~~~
# btrfs qgroup show -p /
qgroupid         rfer         excl parent
--------         ----         ---- ------
0/5          16.00KiB     16.00KiB ---
0/257        16.00KiB     16.00KiB ---
0/258        16.00KiB     16.00KiB ---
0/259         3.59GiB     44.96MiB ---
[...]
0/283         3.57GiB     21.89MiB 1/0
0/287         3.59GiB    512.00KiB 1/0
0/288         3.73GiB      3.88MiB 1/0
[...]
1/0           4.13GiB    830.96MiB ---
~~~

The exclusive space of the parent qgroup 1/0 is now below the limit of 50% for
a 15GiB root filesystem.

You can also list the subvolumes to get the mapping from subvolumes to
qgroups:

~~~
# btrfs subvolume list /
ID 257 gen 288 top level 5 path @
ID 258 gen 315 top level 257 path @/.snapshots
ID 259 gen 315 top level 258 path @/.snapshots/1/snapshot
[...]
ID 283 gen 288 top level 258 path @/.snapshots/2/snapshot
ID 287 gen 313 top level 258 path @/.snapshots/3/snapshot
ID 288 gen 314 top level 258 path @/.snapshots/4/snapshot
~~~

Be warned that snapshots that have no cleanup algorithm set will of course
still not be deleted and so also not count for the space of the parent
qgroup. So if you create such snapshots, e.g. by doing a rollback, you must
take care to delete them manually or set the cleanup algorithm.

Also be warned that btrfs quota support is still under major
development. While implementing the space aware cleanup in snapper several
bugs in the kernel were found and fixed meanwhile. So please try this new
feature on a test machine before using it in production.

This feature is available in snapper since version 0.3.0.
