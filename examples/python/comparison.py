#!/usr/bin/python

import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                         dbus_interface='org.opensuse.Snapper')


config_name = "root"
num_pre = 52
num_post = 53

snapper.CreateComparison(config_name, num_pre, num_post)


files = snapper.GetFiles(config_name, num_pre, num_post)

for file in files:
    print file[0], file[1], file[2]


snapper.SetUndoAll(config_name, num_pre, num_post, True)


undo_steps = snapper.GetUndoSteps("root", num_pre, num_post)

for undo_step in undo_steps:
    print undo_step[0], undo_step[1]

