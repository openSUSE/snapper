#!/usr/bin/python

import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                         dbus_interface='org.opensuse.Snapper')


config = snapper.GetConfig("root")

print config[0], config[1]

for k, v in config[2].items():
    print "%s=%s" % (k, v)

