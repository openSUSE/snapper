#!/usr/bin/python3

from time import sleep
import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                         dbus_interface='org.opensuse.Snapper')


snapper.LockConfig("root")
print("locked")

snapper.LockConfig("root")
snapper.UnlockConfig("root")
print("still locked")

sleep(10)

snapper.UnlockConfig("root")
print("unlocked")

sleep(10)
