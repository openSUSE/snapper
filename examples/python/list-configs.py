#!/usr/bin/python3

import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                         dbus_interface='org.opensuse.Snapper')


configs = snapper.ListConfigs()

for config in configs:
    print(config[0], config[1])
