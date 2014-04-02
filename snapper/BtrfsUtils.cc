/*
 * Copyright (c) [2011-2014] Novell, Inc.
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
#include <btrfs/send-utils.h>
#endif

#include "snapper/Log.h"
#include "snapper/AppUtil.h"
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

#ifndef BTRFS_IOC_SUBVOL_GETFLAGS
#define BTRFS_IOC_SUBVOL_GETFLAGS _IOR(BTRFS_IOCTL_MAGIC, 25, __u64)
#endif


namespace snapper
{


    // See btrfsprogs source code for references.


    bool
    is_subvolume(const struct stat& stat)
    {
	return stat.st_ino == 256 && S_ISDIR(stat.st_mode);
    }


    bool
    is_subvolume_read_only(int fd, bool& read_only)
    {
	__u64 flags;
	if (ioctl(fd, BTRFS_IOC_SUBVOL_GETFLAGS, &flags) != 0)
	    throw runtime_error_with_errno("ioctl(BTRFS_IOC_SUBVOL_GETFLAGS) failed", errno);

	return flags & BTRFS_SUBVOL_RDONLY;
    }


    void
    create_subvolume(int fddst, const string& name)
    {
	struct btrfs_ioctl_vol_args args;
	memset(&args, 0, sizeof(args));

	strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	if (ioctl(fddst, BTRFS_IOC_SUBVOL_CREATE, &args) != 0)
	    throw runtime_error_with_errno("ioctl(BTRFS_IOC_SUBVOL_CREATE) failed", errno);
    }


    void
    create_snapshot(int fd, int fddst, const string& name, bool read_only)
    {
	struct btrfs_ioctl_vol_args_v2 args_v2;
	memset(&args_v2, 0, sizeof(args_v2));

	args_v2.fd = fd;
	args_v2.flags = read_only ? BTRFS_SUBVOL_RDONLY : 0;
	strncpy(args_v2.name, name.c_str(), sizeof(args_v2.name) - 1);

	if (ioctl(fddst, BTRFS_IOC_SNAP_CREATE_V2, &args_v2) == 0)
	    return;
	else if (errno != ENOTTY && errno != EINVAL)
	    throw runtime_error_with_errno("ioctl(BTRFS_IOC_SNAP_CREATE_V2) failed", errno);

	struct btrfs_ioctl_vol_args args;
	memset(&args, 0, sizeof(args));

	args.fd = fd;
	strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	if (ioctl(fddst, BTRFS_IOC_SNAP_CREATE, &args) != 0)
	    throw runtime_error_with_errno("ioctl(BTRFS_IOC_SNAP_CREATE) failed", errno);
    }


    void
    delete_subvolume(int fd, const string& name)
    {
	struct btrfs_ioctl_vol_args args;
	memset(&args, 0, sizeof(args));

	strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	if (ioctl(fd, BTRFS_IOC_SNAP_DESTROY, &args) != 0)
	    throw runtime_error_with_errno("ioctl(BTRFS_IOC_SNAP_DESTROY) failed", errno);
    }


#ifdef ENABLE_ROLLBACK

    void
    set_default_id(int fd, unsigned long long id)
    {
	if (ioctl(fd, BTRFS_IOC_DEFAULT_SUBVOL, &id) != 0)
	    throw runtime_error_with_errno("ioctl(BTRFS_IOC_DEFAULT_SUBVOL) failed", errno);
    }


    unsigned long long
    get_default_id(int fd)
    {
	struct btrfs_ioctl_search_args args;
	memset(&args, 0, sizeof(args));

	struct btrfs_ioctl_search_key* sk = &args.key;
	sk->tree_id = BTRFS_ROOT_TREE_OBJECTID;
	sk->nr_items = 1;
	sk->max_objectid = BTRFS_ROOT_TREE_DIR_OBJECTID;
	sk->min_objectid = BTRFS_ROOT_TREE_DIR_OBJECTID;
	sk->max_type = BTRFS_DIR_ITEM_KEY;
	sk->min_type = BTRFS_DIR_ITEM_KEY;
	sk->max_offset = (__u64) -1;
	sk->max_transid = (__u64) -1;

	if (ioctl(fd, BTRFS_IOC_TREE_SEARCH, &args) != 0)
	    throw runtime_error_with_errno("ioctl(BTRFS_IOC_TREE_SEARCH) failed", errno);

	if (sk->nr_items == 0)
	    throw std::runtime_error("sk->nr_items == 0");

	struct btrfs_ioctl_search_header* sh = (struct btrfs_ioctl_search_header*) args.buf;
	if (sh->type != BTRFS_DIR_ITEM_KEY)
	    throw std::runtime_error("sh->type != BTRFS_DIR_ITEM_KEY");

	struct btrfs_dir_item* di = (struct btrfs_dir_item*)(sh + 1);
	int name_len = btrfs_stack_dir_name_len(di);
	const char* name = (const char*)(di + 1);
	if (strncmp("default", name, name_len) != 0)
	    throw std::runtime_error("name != default");

	return btrfs_disk_key_objectid(&di->location);
    }


    string
    get_subvolume(int fd, unsigned long long id)
    {
	char path[BTRFS_PATH_NAME_MAX + 1];

	if (btrfs_subvolid_resolve(fd, path, sizeof(path), id) != 0)
	    throw std::runtime_error("btrfs_subvolid_resolve failed");

	path[BTRFS_PATH_NAME_MAX] = '\0';
	return path;
    }


    unsigned long long
    get_id(int fd)
    {
	struct btrfs_ioctl_ino_lookup_args args;
	memset(&args, 0, sizeof(args));
	args.treeid = 0;
	args.objectid = BTRFS_FIRST_FREE_OBJECTID;

	if (ioctl(fd, BTRFS_IOC_INO_LOOKUP, &args) != 0)
	    throw runtime_error_with_errno("ioctl(BTRFS_IOC_INO_LOOKUP) failed", errno);

	return args.treeid;
    }

#endif

}
