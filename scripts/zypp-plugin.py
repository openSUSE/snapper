#!/usr/bin/env python

from subprocess import Popen, PIPE
from zypp_plugin import Plugin

class MyPlugin(Plugin):

  def PLUGINBEGIN(self, headers, body):

    args = ["snapper", "create", "--type=pre", "--print-number", "--description=zypp"]
    self.o = Popen(args, stdout=PIPE).communicate()[0].strip()

    self.ack()

  def PLUGINEND(self, headers, body):

    args = ["snapper", "create", "--type=post", "--pre-number=%s" % self.o]
    Popen(args)

    self.ack()

plugin = MyPlugin()
plugin.main()
