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
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_LIBBTRFS
#include <btrfs/ioctl.h>
#endif

#include "snapper/Log.h"
#include "snapper/BtrfsUtils.h"


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

    bool
    is_subvolume(const struct stat& stat)
    {
	// see btrfsprogs source code
	return stat.st_ino == 256 && S_ISDIR(stat.st_mode);
    }


    bool
    create_subvolume(int fddst, const string& name)
    {
	struct btrfs_ioctl_vol_args args;
	memset(&args, 0, sizeof(args));

	strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	return ioctl(fddst, BTRFS_IOC_SUBVOL_CREATE, &args) == 0;
    }


    bool
    create_snapshot(int fd, int fddst, const string& name)
    {
	struct btrfs_ioctl_vol_args_v2 args_v2;
	memset(&args_v2, 0, sizeof(args_v2));

	args_v2.fd = fd;
	args_v2.flags = BTRFS_SUBVOL_RDONLY;
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
    delete_subvolume(int fd, const string& name)
    {
	struct btrfs_ioctl_vol_args args;
	memset(&args, 0, sizeof(args));

	strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	return ioctl(fd, BTRFS_IOC_SNAP_DESTROY, &args) == 0;
    }

}
