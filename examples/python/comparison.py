#!/usr/bin/python

import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.snapper', '/org/opensuse/snapper'),
                         dbus_interface='org.opensuse.snapper')


config_name = "root"
num_pre = 52
num_post = 53

snapper.CreateComparison(config_name, num_pre, num_post)


files = snapper.GetFiles(config_name, num_pre, num_post)

for file in files:
    print file[0], file[1], file[2]


undo = [ [ "/hello", False ], [ "/world", True ] ]

snapper.SetUndo(config_name, num_pre, num_post, undo)


files = snapper.GetFiles("root", num_pre, num_post)

for file in files:
    print file[0], file[1], file[2]


(num_create, num_modify, num_delete) = snapper.GetUndoStatistic(config_name, num_pre, num_post)

print num_create, num_modify, num_delete

