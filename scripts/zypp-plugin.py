#!/usr/bin/env python

from os import readlink, getppid
from os.path import basename
from dbus import SystemBus, Interface
from zypp_plugin import Plugin

class MyPlugin(Plugin):

  def PLUGINBEGIN(self, headers, body):

    exe = basename(readlink("/proc/%d/exe" % getppid()))

    num1 = snapper.CreatePreSnapshot("root", "zypp(%s)" % exe, "number")

    self.ack()

  def PLUGINEND(self, headers, body):

    num2 = snapper.CreatePostSnapshot("root", num1, "", "number")

    self.ack()

bus = SystemBus()

snapper = Interface(get_object('org.opensuse.snapper', '/org/opensuse/snapper'),
                    dbus_interface='org.opensuse.snapper')

plugin = MyPlugin()
plugin.main()
