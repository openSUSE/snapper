#!/usr/bin/python

from time import sleep
import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                         dbus_interface='org.opensuse.Snapper')


snapper.LockConfig("root")

sleep(10)

snapper.UnlockConfig("root")

