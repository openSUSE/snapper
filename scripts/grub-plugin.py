#!/usr/bin/python

from subprocess import check_output
from datetime import datetime
from xattr import xattr
from fcntl import ioctl
from os import open, close, O_RDONLY
from ctypes import c_ulonglong
from sys import argv


BTRFS_IOC_SUBVOL_GETFLAGS = 0x80089419
BTRFS_IOC_SUBVOL_SETFLAGS = 0x4008941a
BTRFS_SUBVOL_RDONLY = 0x2


number = argv[1]

kernel = check_output(["ls", "-1v", "/lib/modules"]).splitlines()[-1]
date = datetime.utcnow()
important = argv[2]


fd = open("/.snapshots/%s/snapshot" % number, O_RDONLY)

orig_buf = c_ulonglong()
ioctl(fd, BTRFS_IOC_SUBVOL_GETFLAGS, orig_buf, True)

new_buf = c_ulonglong(orig_buf.value & ~BTRFS_SUBVOL_RDONLY)
ioctl(fd, BTRFS_IOC_SUBVOL_SETFLAGS, new_buf, True)

x = xattr(fd)

x.set("user.kernel", kernel)
x.set("user.date", date.strftime("%F %T"))
x.set("user.important", important)

ioctl(fd, BTRFS_IOC_SUBVOL_SETFLAGS, orig_buf, True)

close(fd)

