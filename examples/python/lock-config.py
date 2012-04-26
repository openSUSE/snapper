#!/usr/bin/python

from time import sleep
import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.snapper', '/org/opensuse/snapper'),
                         dbus_interface='org.opensuse.snapper')


snapper.LockConfig("root")

sleep(10)

snapper.UnlockConfig("root")

