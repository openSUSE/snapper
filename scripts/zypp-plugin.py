#!/usr/bin/python

from os import readlink, getppid
from os.path import basename
from fnmatch import fnmatch
import logging
from dbus import SystemBus, Interface
import xml.dom.minidom as minidom
import xml.parsers.expat as expat
import json
from zypp_plugin import Plugin



class Solvable:

    def __init__(self, name, important):
        self.name = name
        self.important = important

    def __repr__(self):
        return "name:%s important:%s" % (self.name, self.important)

    def match(self, name):
        return fnmatch(name, self.name)



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
                    self.solvables.append(Solvable(tmp2.childNodes[0].data,
                                                   tmp2.getAttribute("important") == "true"))
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


    def get_solvables(self, body):
        tmp = json.loads(body)
        tsl = tmp["TransactionStepList"]
        solvables = set()
        for ts in tsl:
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

        solvables = self.get_solvables(body)
        logging.debug("solvables: %s" % solvables)

        found, important = self.match_solvables(solvables)
        logging.info("found: %s, important: %s" % (found, important))

        if found or important:

            self.userdata["important"] = "yes" if important else "no"

            logging.info("creating pre snapshot")
            self.num1 = snapper.CreatePreSnapshot("root", self.description, self.cleanup,
                                                  self.userdata)

        self.ack()


    def COMMITEND(self, headers, body):

        logging.info("COMMITEND")

        if self.num1:

            logging.info("creating post snapshot")
            self.num2 = snapper.CreatePostSnapshot("root", self.num1, "", self.cleanup,
                                                   self.userdata)

        self.ack()


    def PLUGINEND(self, headers, body):

        logging.info("PLUGINEND")

        self.ack()



config = Config()

bus = SystemBus()

snapper = Interface(bus.get_object('org.opensuse.Snapper', '/org/opensuse/Snapper'),
                    dbus_interface='org.opensuse.Snapper')

plugin = MyPlugin()
plugin.main()
