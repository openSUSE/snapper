---
title: Introducing snapper&#58; A tool for managing btrfs snapshots
author: Arvin Schnell
layout: post
piwik: true
---

Today we want to present the current development of snapper, a tool
for managing btrfs snapshots.

For years we had the request to provide rollbacks for YaST and zypper
but things never got far due to various technical problems. With the
rise of btrfs snapshots we finally saw the possibility for a usable
solution. The basic idea is to create a snapshot before and after
running YaST or zypper, compare the two snapshots and finally provide
a tool to revert the differences between the two snapshots. That was
the birth of snapper. Soon the idea was extended to create hourly
snapshots as a backup system against general user mistakes.

The tool is now in a state where you can play with it. On the other
hand there is still room and time for modifications and new features.

#### Overview

We provide a command line tool and a YaST UI module. Here is a brief tour:

First we manually create a snapshot:

~~~
# snapper create --description "initial"
~~~

Running YaST automatically creates two snapshots:

~~~
# yast2 users
~~~

Now we can list our snapshots:

~~~
# snapper list
Type   | # | Pre # | Date                     | Cleanup  | Description
-------+---+-------+--------------------------+----------+-------------
single | 0 |       |                          |          | current
single | 1 |       | Wed Mar 30 14:52:17 2011 |          | initial
pre    | 2 |       | Wed Mar 30 14:57:10 2011 | number   | yast users
post   | 3 | 2     | Wed Mar 30 14:57:35 2011 | number   |
single | 4 |       | Wed Mar 30 15:00:01 2011 | timeline | timeline
~~~

Snapshot #0 always refers to the current system. Snapshot #2 and #3
were created by YaST. Snapshot #4 was created by an hourly cronjob.

Getting the list of modified files between to snapshots is easy:

~~~
# snapper diff 2 3
c... /etc/group
c... /etc/group.YaST2save
c... /etc/passwd
c... /etc/passwd.YaST2save
c... /etc/shadow
c... /etc/shadow.YaST2save
c... /etc/sysconfig/displaymanager
c... /var/tmp/kdecache-linux/icon-cache.kcache
c... /var/tmp/kdecache-linux/plasma_theme_openSUSEdefault.kcache
~~~

You can also compare a single file between two snapshots:

~~~
# snapper diff --file /etc/passwd 2 3
--- /snapshots/2/snapshot/etc/passwd    2011-03-30 14:41:45.943000001 +0200
+++ /snapshots/3/snapshot/etc/passwd    2011-03-30 14:57:33.916000003 +0200
@@ -22,3 +22,4 @@
 uucp:x:10:14:Unix-to-Unix CoPy system:/etc/uucp:/bin/bash
 wwwrun:x:30:8:WWW daemon apache:/var/lib/wwwrun:/bin/false
 linux:x:1000:100:linux:/home/linux:/bin/bash
+tux:x:1001:100:tux:/home/tux:/bin/bash
~~~

The main feature of course is to revert the changes between two snapshots:

~~~
# snapper rollback 2 3
~~~

Finally yast2-snapper provides a YaST UI for snapper.

![YaST Snapper](/images/snapper_yast2.png)

#### Testing

Playing with snapper should only be done on test machines. Both btrfs
and snapper are not finished and included known bugs. Here is a
[step-by-step
manual](https://en.opensuse.org/openSUSE:Snapper_install) for
installing and configuring snapper for openSUSE 11.4.

#### Feedback

We would like to get feedback, esp. about general problems and further
ideas. There are also tasks everybody can work on, e.g. the snapper
wiki page or a man-page for snapper.

For the time being there is no dedicated mailing-list so just use
opensuse-factory@opensuse.org.

_Origionally posted on
[lizards.opensuse.org](https://lizards.opensuse.org/2011/04/01/introducing-snapper/)._
