/*
 * Copyright (c) [2011-2013] Novell, Inc.
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


#include "config.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <errno.h>
#include <unistd.h>
#include <mntent.h>
#include <fcntl.h>
#include <asm/types.h>
#include <boost/algorithm/string.hpp>

#include "snapper/Log.h"
#include "snapper/Filesystem.h"
#ifdef ENABLE_BTRFS
#include "snapper/Btrfs.h"
#endif
#ifdef ENABLE_EXT4
#include "snapper/Ext4.h"
#endif
#ifdef ENABLE_LVM
#include "snapper/Lvm.h"
#endif
#include "snapper/Snapper.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Compare.h"


namespace snapper
{

    vector<string>
    Filesystem::filter_mount_options(const vector<string>& options)
    {
	static const char* ign_opt[] = {
	    "ro", "rw",
	    "exec", "noexec", "suid", "nosuid", "dev", "nodev",
	    "atime", "noatime", "diratime", "nodiratime",
	    "relatime", "norelatime", "strictatime", "nostrictatime"
	};

	vector<string> ret = options;

	for (size_t i = 0; i < lengthof(ign_opt); ++i)
	    ret.erase(remove(ret.begin(), ret.end(), ign_opt[i]), ret.end());

	return ret;
    }


    bool
    Filesystem::mount(const string& device, const SDir& dir, const string& mount_type,
		      const vector<string>& options)
    {
	unsigned long mount_flags = MS_RDONLY | MS_NOEXEC | MS_NOSUID | MS_NODEV |
	    MS_NOATIME | MS_NODIRATIME;

	return dir.mount(device, mount_type, mount_flags, boost::join(options, ","));
    }


    bool
    Filesystem::umount(const SDir& dir, const string& mount_point)
    {
	return dir.umount(mount_point);
    }


    Filesystem*
    Filesystem::create(const string& fstype, const string& subvolume)
    {
	typedef Filesystem* (*func_t)(const string& fstype, const string& subvolume);

	static const func_t funcs[] = {
#ifdef ENABLE_BTRFS
		&Btrfs::create,
#endif
#ifdef ENABLE_EXT4
		&Ext4::create,
#endif
#ifdef ENABLE_LVM
		&Lvm::create,
#endif
	NULL };

	for (const func_t* func = funcs; *func != NULL; ++func)
	{
	    Filesystem* fs = (*func)(fstype, subvolume);
	    if (fs)
		return fs;
	}

	y2err("do not know about fstype '" << fstype << "'");
	throw InvalidConfigException();
    }


    SDir
    Filesystem::openSubvolumeDir() const
    {
	SDir subvolume_dir(subvolume);

	return subvolume_dir;
    }


    SDir
    Filesystem::openInfoDir(unsigned int num) const
    {
	SDir infos_dir = openInfosDir();
	SDir info_dir(infos_dir, decString(num));

	return info_dir;
    }


    void
    Filesystem::cmpDirs(const SDir& dir1, const SDir& dir2, cmpdirs_cb_t cb) const
    {
	snapper::cmpDirs(dir1, dir2, cb);
    }

}
