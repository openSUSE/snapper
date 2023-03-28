#!/usr/bin/python3

from time import gmtime, asctime
from pwd import getpwuid
import dbus

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                         dbus_interface='org.opensuse.Snapper')


snapshots = snapper.ListSnapshots("root")

for snapshot in snapshots:
    print(snapshot[0], snapshot[1], snapshot[2], end='')

    if snapshot[3] != -1:
        print("", asctime(gmtime(snapshot[3])), end='')
    else:
        print("", "now", end='')

    print("", getpwuid(snapshot[4])[0], snapshot[5], snapshot[6], end='')
    for k, v in snapshot[7].items():
        print("", "%s=%s" % (k, v), end='')
    print()
