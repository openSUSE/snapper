/*
 * Copyright (c) [2011-2015] Novell, Inc.
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
#define BTRFS_IOC_SYNC _IO(BTRFS_IOCTL_MAGIC, 8)

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

    namespace BtrfsUtils
    {

	// See btrfsprogs source code for references.


	bool
	is_subvolume(const struct stat& stat)
	{
	    return stat.st_ino == 256 && S_ISDIR(stat.st_mode);
	}


	bool
	is_subvolume_read_only(int fd)
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
	create_snapshot(int fd, int fddst, const string& name, bool read_only, qgroup_t qgroup)
	{
	    struct btrfs_ioctl_vol_args_v2 args_v2;
	    memset(&args_v2, 0, sizeof(args_v2));

	    args_v2.fd = fd;
	    args_v2.flags = read_only ? BTRFS_SUBVOL_RDONLY : 0;
	    strncpy(args_v2.name, name.c_str(), sizeof(args_v2.name) - 1);

#ifdef ENABLE_BTRFS_QUOTA

	    if (qgroup != no_qgroup)
	    {
		size_t size = sizeof(btrfs_qgroup_inherit) + sizeof(((btrfs_qgroup_inherit*) 0)->qgroups[0]);
		vector<char> buffer(size, 0);
		struct btrfs_qgroup_inherit* inherit = (btrfs_qgroup_inherit*) &buffer[0];

		inherit->num_qgroups = 1;
		inherit->num_ref_copies = 0;
		inherit->num_excl_copies = 0;
		inherit->qgroups[0] = qgroup;

		args_v2.flags |= BTRFS_SUBVOL_QGROUP_INHERIT;
		args_v2.size = size;
		args_v2.qgroup_inherit = inherit;
	    }

#endif

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
	set_default_id(int fd, subvolid_t id)
	{
	    if (ioctl(fd, BTRFS_IOC_DEFAULT_SUBVOL, &id) != 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_DEFAULT_SUBVOL) failed", errno);
	}


	subvolid_t
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
	get_subvolume(int fd, subvolid_t id)
	{
	    char path[BTRFS_PATH_NAME_MAX + 1];

	    if (btrfs_subvolid_resolve(fd, path, sizeof(path), id) != 0)
		throw std::runtime_error("btrfs_subvolid_resolve failed");

	    path[BTRFS_PATH_NAME_MAX] = '\0';
	    return path;
	}

#endif


#ifdef HAVE_LIBBTRFS

	subvolid_t
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


	bool
	does_subvolume_exist(int fd, subvolid_t subvolid)
	{
	    struct btrfs_ioctl_search_args args;
	    struct btrfs_ioctl_search_key* sk = &args.key;

	    sk->tree_id = BTRFS_ROOT_TREE_OBJECTID;
	    sk->min_objectid = subvolid;
	    sk->max_objectid = subvolid;
	    sk->min_type = BTRFS_ROOT_ITEM_KEY;
	    sk->max_type = BTRFS_ROOT_ITEM_KEY;
	    sk->min_offset = 0;
	    sk->max_offset = (u64) -1;
	    sk->min_transid = 0;
	    sk->max_transid = (u64) -1;
	    sk->nr_items = 1;

	    if (ioctl(fd, BTRFS_IOC_TREE_SEARCH, &args) != 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_TREE_SEARCH) failed", errno);

	    return sk->nr_items == 0;
	}

#endif


#ifdef ENABLE_BTRFS_QUOTA

	void
	quota_enable(int fd)
	{
	     struct btrfs_ioctl_quota_ctl_args args;
	     memset(&args, 0, sizeof(args));
	     args.cmd = BTRFS_QUOTA_CTL_ENABLE;

	     if (ioctl(fd, BTRFS_IOC_QUOTA_CTL, &args) != 0)
		 throw runtime_error_with_errno("ioctl(BTRFS_IOC_QUOTA_CTL) failed", errno);
	}


	void
	quota_disable(int fd)
	{
	     struct btrfs_ioctl_quota_ctl_args args;
	     memset(&args, 0, sizeof(args));
	     args.cmd = BTRFS_QUOTA_CTL_DISABLE;

	     if (ioctl(fd, BTRFS_IOC_QUOTA_CTL, &args) != 0)
		 throw runtime_error_with_errno("ioctl(BTRFS_IOC_QUOTA_CTL) failed", errno);
	}


	void
	quota_rescan(int fd)
	{
	    struct btrfs_ioctl_quota_rescan_args args;
	    memset(&args, 0, sizeof(args));

	    if (ioctl(fd, BTRFS_IOC_QUOTA_RESCAN, &args) != 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_QUOTA_RESCAN) failed", errno);

	    while (true)
	    {
		sleep(1);

		memset(&args, 0, sizeof(args));

		if (ioctl(fd, BTRFS_IOC_QUOTA_RESCAN_STATUS, &args) != 0)
		    throw runtime_error_with_errno("ioctl(BTRFS_IOC_QUOTA_RESCAN_STATUS) failed", errno);

		if (!args.flags)
		    break;
	    }
	}


	qgroup_t
	calc_qgroup(uint64_t level, subvolid_t id)
	{
	    return (level << 48) | id;
	}


	qgroup_t
	parse_qgroup(const string& str)
	{
	    string::size_type pos = str.find('/');
	    if (pos == string::npos)
		throw std::runtime_error("parsing qgroup failed");

	    std::istringstream a(str.substr(0, pos));
	    uint64_t level = 0;
	    a >> level;
	    if (a.fail() || !a.eof())
		throw std::runtime_error("parsing qgroup failed");

	    std::istringstream b(str.substr(pos + 1));
	    subvolid_t id = 0;
	    b >> id;
	    if (b.fail() || !b.eof())
		throw std::runtime_error("parsing qgroup failed");

	    return calc_qgroup(level, id);
	}


	string
	format_qgroup(qgroup_t qgroup)
	{
	    std::ostringstream ret;
	    classic(ret);
	    ret << (qgroup >> 48) << "/" << (qgroup & ((1LLU << 48) - 1));
	    return ret.str();
	}


	void
	qgroup_create(int fd, qgroup_t qgroup)
	{
	    struct btrfs_ioctl_qgroup_create_args args;
	    memset(&args, 0, sizeof(args));
	    args.create = 1;
	    args.qgroupid = qgroup;

	    if (ioctl(fd, BTRFS_IOC_QGROUP_CREATE, &args) != 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_QGROUP_CREATE) failed", errno);
	}


	void
	qgroup_destroy(int fd, qgroup_t qgroup)
	{
	    struct btrfs_ioctl_qgroup_create_args args;
	    memset(&args, 0, sizeof(args));
	    args.create = 0;
	    args.qgroupid = qgroup;

	    if (ioctl(fd, BTRFS_IOC_QGROUP_CREATE, &args) != 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_QGROUP_CREATE) failed", errno);
	}


	void
	qgroup_assign(int fd, qgroup_t src, qgroup_t dst)
	{
	    struct btrfs_ioctl_qgroup_assign_args args;
	    memset(&args, 0, sizeof(args));
	    args.assign = 1;
	    args.src = src;
	    args.dst = dst;

	    if (ioctl(fd, BTRFS_IOC_QGROUP_ASSIGN, &args) != 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_QGROUP_ASSIGN) failed", errno);
	}


	void
	qgroup_remove(int fd, qgroup_t src, qgroup_t dst)
	{
	    struct btrfs_ioctl_qgroup_assign_args args;
	    memset(&args, 0, sizeof(args));
	    args.assign = 0;
	    args.src = src;
	    args.dst = dst;

	    if (ioctl(fd, BTRFS_IOC_QGROUP_ASSIGN, &args) != 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_QGROUP_ASSIGN) failed", errno);
	}


	vector<qgroup_t>
	qgroup_query_children(int fd, qgroup_t qgroup)
	{
	    struct btrfs_ioctl_search_args args;
	    memset(&args, 0, sizeof(args));

	    struct btrfs_ioctl_search_key* sk = &args.key;
	    sk->tree_id = BTRFS_QUOTA_TREE_OBJECTID;

	    sk->min_type = BTRFS_QGROUP_RELATION_KEY;
	    sk->max_type = BTRFS_QGROUP_RELATION_KEY;
	    sk->min_objectid = 0;
	    sk->max_objectid = BTRFS_LAST_FREE_OBJECTID;
	    sk->min_offset = 0;
	    sk->max_offset = (u64)(-1);
	    sk->min_transid = 0;
	    sk->max_transid = (u64)(-1);
	    sk->nr_items = 4096;

	    vector<qgroup_t> ret;

	    while (true)
	    {
		if (ioctl(fd, BTRFS_IOC_TREE_SEARCH, &args) != 0)
		    throw runtime_error_with_errno("ioctl(BTRFS_IOC_TREE_SEARCH) failed", errno);

		if (sk->nr_items == 0)
		    break;

		u64 off = 0;

		for (unsigned int i = 0; i < sk->nr_items; ++i)
		{
		    struct btrfs_ioctl_search_header* sh = (struct btrfs_ioctl_search_header*)(args.buf + off);
		    off += sizeof(*sh);

		    if (sh->type != BTRFS_QGROUP_RELATION_KEY)
			throw std::runtime_error("sh->type != BTRFS_QGROUP_RELATION_KEY");

		    if (sh->offset == qgroup)
			ret.push_back(sh->objectid);

		    off += sh->len;

		    sk->min_objectid = sh->objectid;
		    sk->min_offset = sh->offset;
		}

		sk->nr_items = 4096;

		if (sk->min_offset < (u64)(-1))
		    sk->min_offset++;
		else
		    break;
	    }

	    return ret;
	}


	QGroupUsage
	qgroup_query_usage(int fd, qgroup_t qgroup)
	{
	    struct btrfs_ioctl_search_args args;
	    memset(&args, 0, sizeof(args));

	    struct btrfs_ioctl_search_key* sk = &args.key;
	    sk->tree_id = BTRFS_QUOTA_TREE_OBJECTID;
	    sk->min_objectid = 0;
	    sk->max_objectid = 0;
	    sk->min_offset = qgroup;
	    sk->max_offset = qgroup;
	    sk->min_transid = 0;
	    sk->max_transid = (u64) -1;
	    sk->min_type = BTRFS_QGROUP_INFO_KEY;
	    sk->max_type = BTRFS_QGROUP_INFO_KEY;
	    sk->nr_items = 16;

	    if (ioctl(fd, BTRFS_IOC_TREE_SEARCH, &args) != 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_TREE_SEARCH) failed", errno);

	    if (sk->nr_items != 1)
		throw std::runtime_error("sk->qnr_items != 1");

	    struct btrfs_ioctl_search_header* sh = (struct btrfs_ioctl_search_header*)(args.buf);
	    if (sh->offset != qgroup)
		throw std::runtime_error("sh->offset != qgroup");

	    if (sh->type != BTRFS_QGROUP_INFO_KEY)
		throw std::runtime_error("sh->type != BTRFS_QGROUP_INFO_KEY");

	    struct btrfs_qgroup_info_item info;
	    memcpy(&info, (struct btrfs_qgroup_info_item*)(args.buf + sizeof(*sh)), sizeof(info));

	    QGroupUsage qgroup_usage;
	    qgroup_usage.referenced = le64_to_cpu(info.referenced);
	    qgroup_usage.referenced_compressed = le64_to_cpu(info.referenced_compressed);
	    qgroup_usage.exclusive = le64_to_cpu(info.exclusive);
	    qgroup_usage.exclusive_compressed = le64_to_cpu(info.exclusive_compressed);

	    return qgroup_usage;
	}

#endif


	void
	sync(int fd)
	{
	    if (ioctl(fd, BTRFS_IOC_SYNC) != 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_SYNC) failed", errno);
	}

    }

}
