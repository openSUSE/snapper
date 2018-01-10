#!/usr/bin/python3
#
# Copyright (c) [2011-2014] Novell, Inc.
# Copyright (c) [2015,2018] SUSE LLC
#
# All Rights Reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, contact Novell, Inc.
#
# To contact Novell about this file by physical or electronic mail, you may
# find current contact information at www.novell.com.
#
# Author: Arvin Schnell <aschnell@suse.de>
#


from os import readlink, getppid, environ
from os.path import basename
import sys
import fnmatch
import re
import logging
from dbus import SystemBus, Interface, DBusException
import xml.dom.minidom as minidom
import xml.parsers.expat as expat
import json
from zypp_plugin import Plugin



class Solvable:

    def __init__(self, pattern, important):
        self.pattern = re.compile(pattern)
        self.important = important

    def __repr__(self):
        return "pattern:%s important:%s" % (self.pattern, self.important)

    def match(self, name):
        return self.pattern.match(name)



class Config:

    def __init__(self):
        self.solvables = []
        self.load_file("/etc/snapper/zypp-plugin.conf")


    def load_file(self, filename):
        try:
            self.load_dom(minidom.parse(filename))
        except IOError:
            logging.error("failed to open %s" % filename)
        except expat.ExpatError:
            logging.error("failed to parse %s" % filename)
        except:
            logging.error("failed to load %s" % filename)


    def load_dom(self, dom):
        try:
            for tmp1 in dom.getElementsByTagName("solvables"):
                for tmp2 in tmp1.getElementsByTagName("solvable"):

                    pattern = tmp2.childNodes[0].data
                    match = tmp2.getAttribute("match")
                    important = tmp2.getAttribute("important") == "true"

                    if not match in [ "w", "re" ]:
                        logging.error("unknown match attribute %s" % match)
                        continue

                    if match == "w":
                        pattern = fnmatch.translate(pattern)

                    self.solvables.append(Solvable(pattern, important))

        except:
            pass



class MyPlugin(Plugin):

    def __init__(self):
        Plugin.__init__(self)
        self.num1 = self.num2 = None
        self.description = ""
        self.cleanup = "number"
        self.userdata = {}


    def parse_userdata(self, s):
        userdata = {}
        for kv in s.split(","):
            k, v = kv.split("=", 1)
            k = k.strip()
            if not k:
                raise ValueError
            userdata[k] = v.strip()
        return userdata


    def get_userdata(self, headers):
        try:
            return self.parse_userdata(headers['userdata'])
        except KeyError:
            pass
        except ValueError:
            logging.error("invalid userdata")
        return {}


    def get_solvables(self, body, todo):
        tmp = json.loads(body)
        tsl = tmp["TransactionStepList"]
        solvables = set()
        for ts in tsl:
            if "type" in ts:
                if todo or "stage" in ts:
                    solvables.add(ts["solvable"]["n"])
        return solvables


    def match_solvables(self, names):
        found = important = False
        for name in names:
            for solvable in config.solvables:
                if solvable.match(name):
                    found = True
                    important = important or solvable.important
                    if found and important:
                        return True, True
        return found, important


    def PLUGINBEGIN(self, headers, body):

        logging.info("PLUGINBEGIN")

        logging.debug("headers: %s" % headers)

        self.description = "zypp(%s)" % basename(readlink("/proc/%d/exe" % getppid()))
        self.userdata = self.get_userdata(headers)

        self.ack()


    def COMMITBEGIN(self, headers, body):

        logging.info("COMMITBEGIN")

        solvables = self.get_solvables(body, True)
        logging.debug("solvables: %s" % solvables)

        found, important = self.match_solvables(solvables)
        logging.info("found: %s, important: %s" % (found, important))

        if found or important:

            self.userdata["important"] = "yes" if important else "no"

            try:
                logging.info("creating pre snapshot")
                self.num1 = snapper.CreatePreSnapshot("root", self.description, self.cleanup,
                                                      self.userdata)
                logging.debug("created pre snapshot %d" % self.num1)
            except DBusException as e:
                logging.error("creating snapshot failed:")
                logging.error("  %s", e)

        self.ack()


    def COMMITEND(self, headers, body):

        logging.info("COMMITEND")

        if self.num1:

            solvables = self.get_solvables(body, False)
            logging.debug("solvables: %s" % solvables)

            found, important = self.match_solvables(solvables)
            logging.info("found: %s, important: %s" % (found, important))

            if found or important:

                self.userdata["important"] = "yes" if important else "no"

                try:
                    snapper.SetSnapshot("root", self.num1, self.description, self.cleanup,
                                        self.userdata)
                except DBusException as e:
                    logging.error("setting snapshot data failed:")
                    logging.error("  %s", e)

                try:
                    logging.info("creating post snapshot")
                    self.num2 = snapper.CreatePostSnapshot("root", self.num1, "", self.cleanup,
                                                           self.userdata)
                    logging.debug("created post snapshot %d" % self.num2)
                except DBusException as e:
                    logging.error("creating snapshot failed:")
                    logging.error("  %s", e)

            else:

                try:
                    logging.info("deleting pre snapshot")
                    snapper.DeleteSnapshots("root", [ self.num1 ])
                    logging.debug("deleted pre snapshot %d" % self.num1)
                except DBusException as e:
                    logging.error("deleting snapshot failed:")
                    logging.error("  %s", e)

        self.ack()


    def PLUGINEND(self, headers, body):

        logging.info("PLUGINEND")

        self.ack()


if "DISABLE_SNAPPER_ZYPP_PLUGIN" in environ:

    logging.info("$DISABLE_SNAPPER_ZYPP_PLUGIN is set - disabling snapper-zypp-plugin")

    # a dummy Plugin is needed
    plugin = Plugin()
    plugin.main()

else:

    config = Config()

    try:
        bus = SystemBus()
        snapper = Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                            dbus_interface='org.opensuse.Snapper')
    except DBusException as e:
        logging.error("connect to snapperd failed:")
        logging.error("  %s", e)
        sys.exit(1)

    plugin = MyPlugin()
    plugin.main()
