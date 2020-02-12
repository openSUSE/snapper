---
title: Snapper for Everyone
author: Arvin Schnell
layout: post
---

With the release of snapper 0.1.0 also non-root users are able to
manage snapshots. On the technical side this is achieved by splitting
snapper into a client and server that communicate via
[D-Bus](http://www.freedesktop.org/wiki/Software/dbus). As a user you
should not notice any difference.

So how can you make use of it? Suppose the subvolume /home/tux is
already configured for snapper and you want to allow the user tux to
manage the snapshots for her home directory. This is done in two easy
steps:

1. Edit /etc/snapper/configs/home-tux and add
   ALLOW_USERS=”tux”. Currently the server snapperd does not reload
   the configuration so if it’s running either kill it or wait for it
   to terminate by itself.

2. Give the user permissions to read and access the .snapshots
   directory, 'chmod a+rx /home/tux/.snapshots'.

For details consult the snapper man-page.

Now tux can play with snapper:

~~~
tux> alias snapper="snapper -c home-tux"

tux> snapper create --description test

tux> snapper list
Type   | # | Pre # | Date                             | User | Cleanup  | Description | Userdata
-------+---+-------+----------------------------------+------+----------+-------------+---------
single | 0 |       |                                  | root |          | current     |         
single | 1 |       | Tue 16 Oct 2012 12:15:01 PM CEST | root | timeline | timeline    |         
single | 2 |       | Tue 16 Oct 2012 12:21:38 PM CEST | tux  |          | test        |
~~~

Snapper packages are available for various distributions in the
[filesystems:snapper](https://build.opensuse.org/project/show/filesystems:snapper)
project.

So long and see you at the [openSUSE
Conference](http://conference.opensuse.org/) 2012 in Prague.

_Origionally posted on
[lizards.opensuse.org](https://lizards.opensuse.org/2012/10/16/snapper-for-everyone/)._
