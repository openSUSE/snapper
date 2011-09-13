#!/usr/bin/python

import libsnapper


sh = libsnapper.createSnapper("root", False)

sh.createSingleSnapshot("python test")

libsnapper.deleteSnapper(sh)

