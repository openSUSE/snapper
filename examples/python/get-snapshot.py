#!/usr/bin/python

from time import gmtime, asctime
from pwd import getpwuid
import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                         dbus_interface='org.opensuse.Snapper')


snapshot = snapper.GetSnapshot("root", 1)

print snapshot[0], snapshot[1], snapshot[2], asctime(gmtime(snapshot[3])),
print getpwuid(snapshot[4])[0], snapshot[5], snapshot[6],
for k, v in snapshot[7].items():
    print "%s=%s" % (k, v),
print

