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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#ifdef HAVE_LIBBTRFS
#include <btrfs/ioctl.h>
#endif
#include <boost/algorithm/string.hpp>

#include "snapper/Log.h"
#include "snapper/Btrfs.h"
#include "snapper/Snapper.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SnapperDefines.h"


#ifndef HAVE_LIBBTRFS

#define BTRFS_IOCTL_MAGIC 0x94
#define BTRFS_PATH_NAME_MAX 4087
#define BTRFS_SUBVOL_NAME_MAX 4039
#define BTRFS_SUBVOL_RDONLY (1ULL << 1)

#define BTRFS_IOC_SNAP_CREATE _IOW(BTRFS_IOCTL_MAGIC, 1, struct btrfs_ioctl_vol_args)
#define BTRFS_IOC_SUBVOL_CREATE _IOW(BTRFS_IOCTL_MAGIC, 14, struct btrfs_ioctl_vol_args)
#define BTRFS_IOC_SNAP_DESTROY _IOW(BTRFS_IOCTL_MAGIC, 15, struct btrfs_ioctl_vol_args)
#define BTRFS_IOC_SNAP_CREATE_V2 _IOW(BTRFS_IOCTL_MAGIC, 23, struct btrfs_ioctl_vol_args_v2)

struct btrfs_ioctl_vol_args
{
    __s64 fd;
    char name[BTRFS_PATH_NAME_MAX + 1];
};

struct btrfs_ioctl_vol_args_v2
{
    __s64 fd;
    __u64 transid;
    __u64 flags;
    __u64 unused[4];
    char name[BTRFS_SUBVOL_NAME_MAX + 1];
};

#endif


namespace snapper
{

    Filesystem*
    Btrfs::create(const string& fstype, const string& subvolume)
    {
	if (fstype == "btrfs")
	    return new Btrfs(subvolume);

	return NULL;
    }


    Btrfs::Btrfs(const string& subvolume)
	: Filesystem(subvolume)
    {
    }


    void
    Btrfs::createConfig() const
    {
	SDir subvolume_dir = openSubvolumeDir();

	if (!create_subvolume(subvolume_dir.fd(), ".snapshots"))
	{
	    y2err("create subvolume failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw CreateConfigFailedException("creating btrfs snapshot failed");
	}
    }


    void
    Btrfs::deleteConfig() const
    {
	SDir subvolume_dir = openSubvolumeDir();

	if (!delete_subvolume(subvolume_dir.fd(), ".snapshots"))
	{
	    y2err("delete subvolume failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw DeleteConfigFailedException("deleting btrfs snapshot failed");
	}
    }


    string
    Btrfs::snapshotDir(unsigned int num) const
    {
	return (subvolume == "/" ? "" : subvolume) + "/.snapshots/" + decString(num) +
	    "/snapshot";
    }


    SDir
    Btrfs::openSubvolumeDir() const
    {
	SDir subvolume_dir = Filesystem::openSubvolumeDir();

	struct stat stat;
	if (subvolume_dir.stat(&stat) != 0)
	{
	    throw IOErrorException();
	}

	if (!is_subvolume(stat))
	{
	    y2err("subvolume is not a btrfs snapshot");
	    throw IOErrorException();
	}

	return subvolume_dir;
    }


    SDir
    Btrfs::openInfosDir() const
    {
	SDir subvolume_dir = openSubvolumeDir();
	SDir infos_dir(subvolume_dir, ".snapshots");

	struct stat stat;
	if (infos_dir.stat(&stat) != 0)
	{
	    throw IOErrorException();
	}

	if (!is_subvolume(stat))
	{
	    y2err(".snapshots is not a btrfs snapshot");
	    throw IOErrorException();
	}

	if (stat.st_uid != 0)
	{
	    y2err(".snapshots must have owner root");
	    throw IOErrorException();
	}

	if (stat.st_gid != 0 && stat.st_mode & S_IWGRP)
	{
	    y2err(".snapshots must have group root or must not be group-writable");
	    throw IOErrorException();
	}

	if (stat.st_mode & S_IWOTH)
	{
	    y2err(".snapshots must not be world-writable");
	    throw IOErrorException();
	}

	return infos_dir;
    }


    SDir
    Btrfs::openSnapshotDir(unsigned int num) const
    {
	SDir info_dir = openInfoDir(num);
	SDir snapshot_dir(info_dir, "snapshot");

	return snapshot_dir;
    }


    void
    Btrfs::createSnapshot(unsigned int num) const
    {
	SDir subvolume_dir = openSubvolumeDir();
	SDir info_dir = openInfoDir(num);

	if (!create_snapshot(subvolume_dir.fd(), info_dir.fd(), "snapshot"))
	{
	    y2err("create snapshot failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw CreateSnapshotFailedException();
	}
    }


    void
    Btrfs::deleteSnapshot(unsigned int num) const
    {
	SDir info_dir = openInfoDir(num);

	if (!delete_subvolume(info_dir.fd(), "snapshot"))
	{
	    y2err("delete snapshot failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw DeleteSnapshotFailedException();
	}
    }


    bool
    Btrfs::isSnapshotMounted(unsigned int num) const
    {
	return true;
    }


    void
    Btrfs::mountSnapshot(unsigned int num) const
    {
    }


    void
    Btrfs::umountSnapshot(unsigned int num) const
    {
    }


    bool
    Btrfs::checkSnapshot(unsigned int num) const
    {
	try
	{
	    SDir info_dir = openInfoDir(num);

	    struct stat stat;
	    int r = info_dir.stat("snapshot", &stat, AT_SYMLINK_NOFOLLOW);
	    return r == 0 && is_subvolume(stat);
	}
	catch (const IOErrorException& e)
	{
	    return false;
	}
    }


    bool
    Btrfs::is_subvolume(const struct stat& stat) const
    {
	// see btrfsprogs source code
	return stat.st_ino == 256 && S_ISDIR(stat.st_mode);
    }


    bool
    Btrfs::create_subvolume(int fddst, const string& name) const
    {
	struct btrfs_ioctl_vol_args args;
	memset(&args, 0, sizeof(args));

	strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	return ioctl(fddst, BTRFS_IOC_SUBVOL_CREATE, &args) == 0;
    }


    bool
    Btrfs::create_snapshot(int fd, int fddst, const string& name) const
    {
	struct btrfs_ioctl_vol_args_v2 args_v2;
	memset(&args_v2, 0, sizeof(args_v2));

	args_v2.fd = fd;
	args_v2.flags |= BTRFS_SUBVOL_RDONLY;
	strncpy(args_v2.name, name.c_str(), sizeof(args_v2.name) - 1);

	if (ioctl(fddst, BTRFS_IOC_SNAP_CREATE_V2, &args_v2) == 0)
	    return true;
	else if (errno != ENOTTY && errno != EINVAL)
	    return false;

	struct btrfs_ioctl_vol_args args;
	memset(&args, 0, sizeof(args));

	args.fd = fd;
	strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	return ioctl(fddst, BTRFS_IOC_SNAP_CREATE, &args) == 0;
    }


    bool
    Btrfs::delete_subvolume(int fd, const string& name) const
    {
	struct btrfs_ioctl_vol_args args;
	memset(&args, 0, sizeof(args));

	strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	return ioctl(fd, BTRFS_IOC_SNAP_DESTROY, &args) == 0;
    }

}
