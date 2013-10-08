#!/usr/bin/python

from os import readlink, getppid
from os.path import basename
from sys import stderr
from dbus import SystemBus, Interface
from zypp_plugin import Plugin


class MyPlugin(Plugin):


  def parse_userdata(self, s):
    ud = {}
    for kv in s.split(","):
      k, v = kv.split("=", 1)
      k = k.strip()
      if not k:
        raise ValueError
      ud[k] = v.strip()
    return ud


  def PLUGINBEGIN(self, headers, body):

    exe = basename(readlink("/proc/%d/exe" % getppid()))

    self.userdata = {}

    try:
      self.userdata = self.parse_userdata(headers['userdata'])
    except KeyError:
      pass
    except ValueError:
      stderr.write("invalid userdata")

    self.num1 = snapper.CreatePreSnapshot("root", "zypp(%s)" % exe, "number", self.userdata)

    self.ack()


  def PLUGINEND(self, headers, body):

    self.num2 = snapper.CreatePostSnapshot("root", self.num1, "", "number", self.userdata)

    self.ack()


bus = SystemBus()

snapper = Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                    dbus_interface='org.opensuse.Snapper')

plugin = MyPlugin()
plugin.main()
