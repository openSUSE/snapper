/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2018] SUSE LLC
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
	static const char* ign_opts[] = {
	    "ro", "rw",
	    "exec", "noexec", "suid", "nosuid", "dev", "nodev",
	    "atime", "noatime", "diratime", "nodiratime",
	    "relatime", "norelatime", "strictatime", "nostrictatime"
	};

	vector<string> ret = options;

	for (const char* ign_opt : ign_opts)
	    ret.erase(remove(ret.begin(), ret.end(), ign_opt), ret.end());

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
    Filesystem::create(const string& fstype, const string& subvolume, const string& root_prefix)
    {
	typedef Filesystem* (*func_t)(const string& fstype, const string& subvolume,
				      const string& root_prefix);

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
		NULL
	};

	for (const func_t* func = funcs; *func != NULL; ++func)
	{
	    Filesystem* fs = (*func)(fstype, subvolume, root_prefix);
	    if (fs)
		return fs;
	}

	y2err("do not know about fstype '" << fstype << "'");
	SN_THROW(InvalidConfigException());
	__builtin_unreachable();
    }


    Filesystem*
    Filesystem::create(const ConfigInfo& config_info, const string& root_prefix)
    {
	string fstype = "btrfs";
	config_info.getValue(KEY_FSTYPE, fstype);

	Filesystem* fs = create(fstype, config_info.getSubvolume(), root_prefix);

	fs->evalConfigInfo(config_info);

	return fs;
    }


    SDir
    Filesystem::openSubvolumeDir() const
    {
	SDir subvolume_dir(prepend_root_prefix(root_prefix, subvolume));

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


    void
    Filesystem::createSnapshotOfDefault(unsigned int num, bool read_only, bool quota) const
    {
	SN_THROW(UnsupportedException());
	__builtin_unreachable();
    }


    bool
    Filesystem::isDefault(unsigned int num) const
    {
	return false;
    }


    std::pair<bool, unsigned int>
    Filesystem::getDefault() const
    {
	return std::make_pair(false, 0);
    }


    void
    Filesystem::setDefault(unsigned int num) const
    {
	SN_THROW(UnsupportedException());
	__builtin_unreachable();
    }


    std::pair<bool, unsigned int>
    Filesystem::getActive() const
    {
	return std::make_pair(false, 0);
    }


    bool
    Filesystem::isActive(unsigned int num) const
    {
	return false;
    }


    void
    Filesystem::sync() const
    {
    }

}
