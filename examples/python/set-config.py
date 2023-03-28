#!/usr/bin/python3

import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                         dbus_interface='org.opensuse.Snapper')


data = { "NUMBER_CLEANUP" : "yes", "NUMBER_LIMIT" : "10" }

snapper.SetConfig("root", data)
