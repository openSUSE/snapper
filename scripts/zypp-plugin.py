#!/usr/bin/env python

from os import readlink, getppid
from os.path import basename
from subprocess import Popen, PIPE
from zypp_plugin import Plugin

class MyPlugin(Plugin):

  def PLUGINBEGIN(self, headers, body):

    exe = basename(readlink("/proc/%d/exe" % getppid()))

    args = ["snapper", "create", "--type=pre", "--print-number",
            "--cleanup-algorithm=number", "--description=zypp(%s)" % exe]
    self.o = Popen(args, stdout=PIPE).communicate()[0].strip()

    self.ack()

  def PLUGINEND(self, headers, body):

    args = ["snapper", "create", "--type=post", "--pre-number=%s" % self.o,
            "--cleanup-algorithm=number"]
    Popen(args)

    self.ack()

plugin = MyPlugin()
plugin.main()
