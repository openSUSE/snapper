---
title: All-Time Unique Snapshot Numbers
author: Arvin Schnell
layout: post
---

In snapper version 0.13.1, I introduced all-time unique snapshot
numbers. This feature ensures that once a snapshot number has been
used, it is never reused for a new snapshot, even if the original
snapshot is deleted. This was done to make tracking snapshots easier
and to prevent issues in scripts and backup tools like snbk.

However, some users prefer the traditional behavior where snapper
reuses the lowest next available snapshot number.

To accommodate this preference, there is now an option to disable
all-time unique snapshot numbers. You can return to the traditional
behavior by setting `UNIQUE_NUMBERS` to "no" in your snapper
configuration file:

~~~
snapper set-config UNIQUE_NUMBERS=no
~~~

The default value is "yes". If you set it to "no", snapper will fall
back to its previous behavior of reusing deleted snapshot numbers.

All-time unique snapshot numbers are available in snapper since
version 0.13.1. The UNIQUE_NUMBERS parameter will be available in
snapper version 0.13.2.
