---
title: Work without DBus
author: Arvin Schnell
layout: post
---

Normally the _snapper_ command line tool uses <a
href="https://www.freedesktop.org/wiki/Software/dbus/">DBus</a> to connect to
_snapperd_ which does most of the actual work. This allows non-root users to
work with _snapper_.

There are however situations when using DBus is not possible, e.g. chrooted on
the rescue system or when DBus itself is broken after an update. Since snapper
is also a disaster recovery, tool this can limit its usefulness. So some
snapper commands supported the --no-dbus option, bypassing DBus and
snapperd. Now with the latest version all snapper commands support the
--no-dbus option.

Those readers interested in implementation details can continue.

Commands supporting --no-dbus so far were basically implemented by using two
different code paths, one with and one without DBus.

{% highlight c++ %}

if (no_dbus)
{
    Snapper snapper(config_name, target_root);

    SCD scd;
    scd.description = "test";

    Snapshots::const_iterator snapshot = snapper.createSingleSnapshot(scd);

    if (print_number)
        cout << snapshot->getNum() << '\n';
}
else
{
    DBus::Connection conn(DBUS_BUS_SYSTEM);

    unsigned int num = command_create_single_snapshot(conn, config_name, "test", "", {});

    if (print_number)
        cout << num << '\n';
}

{% endhighlight %}

That code is obviously not <a
href="https://en.wikipedia.org/wiki/Don't_repeat_yourself">"dry"</a> and thus
is error-prone.

The new approach solves this problem by adding another abstraction layer in
form of proxy classes that allow to use the same interface for both cases.

{% highlight c++ %}

ProxySnappers snappers(no_dbus ? ProxySnappers::createLib(target_root) : ProxySnappers::createDbus());

ProxySnapper* snapper = snappers.getSnapper(config_name);

SCD scd;
scd.description = "test";

ProxySnapshots::const_iterator snapshot = snapper->createSingleSnapshot(scd);

if (print_number)
    cout << snapshot->getNum() << '\n';

{% endhighlight %}

So only the command to generate the ProxySnappers object depends on the
--no-dbus option.

This feature is available in snapper since version 0.4.0.
