#!/usr/bin/python

import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.snapper', '/org/opensuse/snapper'),
                         dbus_interface='org.opensuse.snapper')


num_files = snapper.CreateComparison("root", 1306, 1307)

print num_files


files = snapper.GetFiles("root", 1306, 1307)

for file in files:
    print file[0], file[1], file[2]


undo = [ [ "/hello", False ], [ "/world", True ] ]

snapper.SetUndo("root", 1306, 1307, undo)


files = snapper.GetFiles("root", 1306, 1307)

for file in files:
    print file[0], file[1], file[2]

