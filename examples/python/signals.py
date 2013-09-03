#!/usr/bin/python

import dbus
from dbus.mainloop.glib import DBusGMainLoop
from gobject import MainLoop


DBusGMainLoop(set_as_default=True)

bus = dbus.SystemBus()

snapper = dbus.Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                         dbus_interface='org.opensuse.Snapper')


class MessageListener:

    def __init__(self):

        signals = {
            "ConfigCreated" : self.config_created,
            "ConfigModified" : self.config_modified,
            "ConfigDeleted" : self.config_deleted,
            "SnapshotCreated" : self.snapshot_created,
            "SnapshotModified" : self.snapshot_modified,
            "SnapshotsDeleted" : self.snapshots_deleted,
        }

        for signal, handler in signals.items():
            snapper.connect_to_signal(signal, handler)


    def config_created(self, config):
        print "ConfigCreated", config

    def config_modified(self, config):
        print "ConfigModified", config

    def config_deleted(self, config):
        print "ConfigDeleted", config

    def snapshot_created(self, config, number):
        print "SnapshotCreated", config, number

    def snapshot_modified(self, config, number):
        print "SnapshotModified", config, number

    def snapshots_deleted(self, config, numbers):
        print "SnapshotsDeleted", config,
        for number in numbers:
            print number,
        print


MessageListener()

MainLoop().run()

