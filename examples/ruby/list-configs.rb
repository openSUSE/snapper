#!/usr/bin/ruby

require "dbus"

system_bus = DBus::SystemBus.instance

service = system_bus.service("org.opensuse.Snapper")

dbus_object = service.object("/org/opensuse/Snapper")

dbus_object.introspect
dbus_object.default_iface = "org.opensuse.Snapper"

configs = dbus_object.send("ListConfigs")[0]

configs.each do |config|
  print config[0], " ", config[1], "\n"
end
