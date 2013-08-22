#!/usr/bin/ruby

require "dbus"

system_bus = DBus::SystemBus.instance

service = system_bus.service("org.opensuse.Snapper")

dbus_object = service.object("/org/opensuse/Snapper")

dbus_object.introspect
dbus_object.default_iface = "org.opensuse.Snapper"

dbus_object.send("CreateComparison", "root", 1, 2)

files = dbus_object.send("GetFiles", "root", 1, 2)[0]

files.each do |file|
  print file[0], " ", file[1], "\n"
end
