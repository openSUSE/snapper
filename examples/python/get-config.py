#!/usr/bin/python

import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.snapper', '/org/opensuse/snapper'),
                         dbus_interface='org.opensuse.snapper')


config = snapper.GetConfig("root")

for k, v in config.items():
    print "%s=%s" % (k, v)

