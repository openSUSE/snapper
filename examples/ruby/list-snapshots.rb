#!/usr/bin/ruby

require "dbus"

system_bus = DBus::SystemBus.instance

service = system_bus.service("org.opensuse.Snapper")

dbus_object = service.object("/org/opensuse/Snapper")

dbus_object.introspect
dbus_object.default_iface = "org.opensuse.Snapper"

snapshots = dbus_object.send("ListSnapshots", "root")[0]

snapshots.each do |snapshot|
  print snapshot, "\n"
end

