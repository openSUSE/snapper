---
title: Snapper with Read-Only Btrfs Root Filesystem
author: Arvin Schnell
layout: post
piwik: true
---

Snapper was just improved to work with a read-only btrfs root filesystem. You
may ask who needs that. Well, SUSE needs it for the new
[CaaSP](https://www.suse.com/communities/blog/introducing-suse-containers-service-platform/)
product, which has a read-only root filesystem and uses transactional updates.

Transactional updates work by creating a snapshot of the system and then
updating the packages in that new snapshot. This way the update does not
affect any of the running services. Instead the update is activated by a
reboot. A read-only root filesystem on a system with transactional updates
simply ensures that the update does not accidentally modify the running
system.

For more information read the [announcement of transactional
updated](https://lists.opensuse.org/opensuse-factory/2017-01/msg00367.html).

This feature is available in snapper since version 0.4.3.
