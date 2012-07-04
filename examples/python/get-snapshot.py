#!/usr/bin/python

import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.snapper', '/org/opensuse/snapper'),
                         dbus_interface='org.opensuse.snapper')


snapshot = snapper.GetSnapshot("root", 1)

print snapshot[0], snapshot[1], snapshot[2], snapshot[3], snapshot[4],
for k, v in snapshot[5].items():
    print "%s=%s" % (k, v),
print
