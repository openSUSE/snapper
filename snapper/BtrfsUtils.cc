/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2023] SUSE LLC
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

#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_LIBBTRFS
#include <btrfs/ioctl.h>
#include <btrfs/send-utils.h>
#else
#include <linux/btrfs.h>
#endif
#ifdef HAVE_LIBBTRFSUTIL
#include <btrfsutil.h>
#endif
#include <algorithm>
#include <functional>

#include "snapper/Log.h"
#include "snapper/AppUtil.h"
#include "snapper/BtrfsUtils.h"


namespace snapper
{

    namespace BtrfsUtils
    {

	// For general information about btrfs see
	// https://btrfs.wiki.kernel.org/ and for more information about btrfs
	// quota groups see http://sensille.com/qgroups.pdf.  For ioctls see
	// btrfsprogs source code for references.


	bool
	is_subvolume(const struct stat& stat)
	{
	    return stat.st_ino == 256 && S_ISDIR(stat.st_mode);
	}


	bool
	is_subvolume_read_only(int fd)
	{
#ifdef HAVE_LIBBTRFSUTIL
	    enum btrfs_util_error err;
	    bool read_only;

	    err = btrfs_util_get_subvolume_read_only_fd(fd, &read_only);
	    if (err)
		throw runtime_error_with_errno("btrfs_util_get_subvolume_read_only_fd() failed", errno);

	    return read_only;
#else
	    __u64 flags;
	    if (ioctl(fd, BTRFS_IOC_SUBVOL_GETFLAGS, &flags) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_SUBVOL_GETFLAGS) failed", errno);

	    return flags & BTRFS_SUBVOL_RDONLY;
#endif
	}


	void
	set_subvolume_read_only(int fd, bool read_only)
	{
#ifdef HAVE_LIBBTRFSUTIL
	    enum btrfs_util_error err;

	    err = btrfs_util_set_subvolume_read_only_fd(fd, read_only);
	    if (err)
		throw runtime_error_with_errno("btrfs_util_set_subvolume_read_only_fd() failed", errno);
#else
	    __u64 flags;
	    if (ioctl(fd, BTRFS_IOC_SUBVOL_GETFLAGS, &flags) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_SUBVOL_GETFLAGS) failed", errno);

	    if (read_only)
		flags |= BTRFS_SUBVOL_RDONLY;
	    else
		flags &= ~BTRFS_SUBVOL_RDONLY;

	    if (ioctl(fd, BTRFS_IOC_SUBVOL_SETFLAGS, &flags) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_SUBVOL_SETFLAGS) failed", errno);
#endif
	}


	void
	create_subvolume(int fddst, const string& name)
	{
#ifdef HAVE_LIBBTRFSUTIL
	    enum btrfs_util_error err;

	    err = btrfs_util_create_subvolume_fd(fddst, name.c_str(), 0, NULL, NULL);
	    if (err)
		throw runtime_error_with_errno("btrfs_util_create_subvolume_fd() failed", errno);
#else
	    struct btrfs_ioctl_vol_args args;
	    memset(&args, 0, sizeof(args));

	    strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	    if (ioctl(fddst, BTRFS_IOC_SUBVOL_CREATE, &args) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_SUBVOL_CREATE) failed", errno);
#endif
	}


	void
	create_snapshot(int fd, int fddst, const string& name, bool read_only, qgroup_t qgroup)
	{
#ifdef HAVE_LIBBTRFSUTIL
	    struct btrfs_util_qgroup_inherit *util_inherit = NULL;
	    int flags = 0;

	    if (read_only)
		flags |= BTRFS_UTIL_CREATE_SNAPSHOT_READ_ONLY;

#ifdef ENABLE_BTRFS_QUOTA
	    size_t size = sizeof(btrfs_qgroup_inherit) + sizeof(((btrfs_qgroup_inherit*) 0)->qgroups[0]);
	    vector<char> buffer(size, 0);

	    if (qgroup != no_qgroup)
	    {
		struct btrfs_qgroup_inherit *inherit;

		inherit = (btrfs_qgroup_inherit*) &buffer[0];
		inherit->num_qgroups = 1;
		inherit->num_ref_copies = 0;
		inherit->num_excl_copies = 0;
		inherit->qgroups[0] = qgroup;
		util_inherit = (struct btrfs_util_qgroup_inherit *)inherit;
	    }
#endif
	    enum btrfs_util_error err = btrfs_util_create_snapshot_fd2(fd, fddst, name.c_str(), flags, NULL,
								       util_inherit);
	    if (!err)
		return;
	    else if (errno != ENOTTY && errno != EINVAL)
		throw runtime_error_with_errno("btrfs_util_create_snapshot_fd2() failed", errno);

#else
	    struct btrfs_ioctl_vol_args_v2 args_v2;
	    memset(&args_v2, 0, sizeof(args_v2));

	    args_v2.fd = fd;
	    args_v2.flags = read_only ? BTRFS_SUBVOL_RDONLY : 0;
	    strncpy(args_v2.name, name.c_str(), sizeof(args_v2.name) - 1);

#ifdef ENABLE_BTRFS_QUOTA

	    size_t size = sizeof(btrfs_qgroup_inherit) + sizeof(((btrfs_qgroup_inherit*) 0)->qgroups[0]);
	    vector<char> buffer(size, 0);

	    if (qgroup != no_qgroup)
	    {
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

#endif

	    struct btrfs_ioctl_vol_args args;
	    memset(&args, 0, sizeof(args));

	    args.fd = fd;
	    strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	    if (ioctl(fddst, BTRFS_IOC_SNAP_CREATE, &args) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_SNAP_CREATE) failed", errno);
	}


	void
	delete_subvolume(int fd, const string& name)
	{
#ifdef HAVE_LIBBTRFSUTIL
	    enum btrfs_util_error err;

	    err = btrfs_util_delete_subvolume_fd(fd, name.c_str(), 0);
	    if (err)
		throw runtime_error_with_errno("btrfs_util_delete_subvolume_fd() failed", errno);
#else
	    struct btrfs_ioctl_vol_args args;
	    memset(&args, 0, sizeof(args));

	    strncpy(args.name, name.c_str(), sizeof(args.name) - 1);

	    if (ioctl(fd, BTRFS_IOC_SNAP_DESTROY, &args) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_SNAP_DESTROY) failed", errno);
#endif
	}


#ifdef ENABLE_ROLLBACK

	void
	set_default_id(int fd, subvolid_t id)
	{
#ifdef HAVE_LIBBTRFSUTIL
	    enum btrfs_util_error err;

	    err = btrfs_util_set_default_subvolume_fd(fd, id);
	    if (err)
		throw runtime_error_with_errno("btrfs_util_set_default_subvolume_fd() failed", errno);
#else
	    if (ioctl(fd, BTRFS_IOC_DEFAULT_SUBVOL, &id) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_DEFAULT_SUBVOL) failed", errno);
#endif
	}


	subvolid_t
	get_default_id(int fd)
	{
#ifdef HAVE_LIBBTRFSUTIL
	    enum btrfs_util_error err;
	    uint64_t id;

	    err = btrfs_util_get_default_subvolume_fd(fd, &id);
	    if (err)
		throw runtime_error_with_errno("btrfs_util_get_default_subvolume_fd() failed", errno);

	    return id;
#else
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

	    if (ioctl(fd, BTRFS_IOC_TREE_SEARCH, &args) < 0)
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
#endif
	}


	string
	get_subvolume(int fd, subvolid_t id)
	{
#ifdef HAVE_LIBBTRFSUTIL
	    enum btrfs_util_error err;
	    char *tmp;
	    string path;

	    /* This requires CAP_SYS_ADMIN as it uses the TREE_SEARCH ioctl but is fast. */
	    err = btrfs_util_subvolume_path_fd(fd, id, &tmp);
	    if (err == BTRFS_UTIL_ERROR_SUBVOLUME_NOT_FOUND)
		throw runtime_error_with_errno("btrfs_util_subvolume_path_fd() failed", errno);

	    /* Try slower iterative search but without restrictions. */
	    if (err == BTRFS_UTIL_OK) {
		path = tmp;
		free(tmp);
	    } else if (err == BTRFS_UTIL_ERROR_SEARCH_FAILED || err == BTRFS_UTIL_ERROR_NO_MEMORY) {
		struct btrfs_util_subvolume_iterator *iter;

		err = btrfs_util_create_subvolume_iterator_fd(fd, 0, 0, &iter);
		if (err)
		    throw runtime_error_with_errno("btrfs_util_subvolume_path_fd() failed", errno);

		while (1) {
		    struct btrfs_util_subvolume_info subvol;

		    err = btrfs_util_subvolume_iterator_next_info(iter, &tmp, &subvol);
		    if (err != BTRFS_UTIL_OK) {
			/* Nothing found or other error */
			btrfs_util_destroy_subvolume_iterator(iter);
			throw std::runtime_error("get_subvolume() failed");
		    }

		    if (subvol.id == id) {
			btrfs_util_destroy_subvolume_iterator(iter);
			path = tmp;
			free(tmp);
			break;
		    }
		    free(tmp);
		}
	    } else {
		/* Unknown error */
		throw std::runtime_error("get_subvolume() failed");
	    }

	    return path;
#else
	    char path[BTRFS_PATH_NAME_MAX + 1];

	    if (btrfs_subvolid_resolve(fd, path, sizeof(path), id) != 0)
		throw std::runtime_error("btrfs_subvolid_resolve failed");

	    path[BTRFS_PATH_NAME_MAX] = '\0';
	    return path;
#endif
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

	    if (ioctl(fd, BTRFS_IOC_INO_LOOKUP, &args) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_INO_LOOKUP) failed", errno);

	    return args.treeid;
	}


	bool
	does_subvolume_exist(int fd, subvolid_t subvolid)
	{
#ifdef HAVE_LIBBTRFSUTIL
	    enum btrfs_util_error err;
	    struct btrfs_util_subvolume_info subvol_info;

	    err = btrfs_util_subvolume_info_fd(fd, subvolid, &subvol_info);
	    if (err == BTRFS_UTIL_ERROR_SUBVOLUME_NOT_FOUND)
		return false;
	    else if (err)
		throw runtime_error_with_errno("btrfs_util_subvolume_info_fd() failed", errno);

	    return true;
#else
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

	    if (ioctl(fd, BTRFS_IOC_TREE_SEARCH, &args) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_TREE_SEARCH) failed", errno);

	    return sk->nr_items > 0;
#endif
	}

#endif


#ifdef ENABLE_BTRFS_QUOTA

	void
	quota_enable(int fd)
	{
	     struct btrfs_ioctl_quota_ctl_args args;
	     memset(&args, 0, sizeof(args));
	     args.cmd = BTRFS_QUOTA_CTL_ENABLE;

	     if (ioctl(fd, BTRFS_IOC_QUOTA_CTL, &args) < 0)
		 throw runtime_error_with_errno("ioctl(BTRFS_IOC_QUOTA_CTL) failed", errno);
	}


	void
	quota_disable(int fd)
	{
	     struct btrfs_ioctl_quota_ctl_args args;
	     memset(&args, 0, sizeof(args));
	     args.cmd = BTRFS_QUOTA_CTL_DISABLE;

	     if (ioctl(fd, BTRFS_IOC_QUOTA_CTL, &args) < 0)
		 throw runtime_error_with_errno("ioctl(BTRFS_IOC_QUOTA_CTL) failed", errno);
	}


	void
	quota_rescan(int fd)
	{
	    struct btrfs_ioctl_quota_rescan_args args;
	    memset(&args, 0, sizeof(args));

	    for (int i = 0;; ++i)
	    {
		if (ioctl(fd, BTRFS_IOC_QUOTA_RESCAN, &args) == 0)
		    break;

		if (errno == EINPROGRESS)
		{
		    if (i == 0)
			y2war("waiting for old quota rescan to finish");

		    sleep(1);
		    continue;
		}

		throw runtime_error_with_errno("ioctl(BTRFS_IOC_QUOTA_RESCAN) failed", errno);
	    }

	    if (ioctl(fd, BTRFS_IOC_QUOTA_RESCAN_WAIT, &args) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_QUOTA_WAIT_RESCAN) failed", errno);
	}


	qgroup_t
	calc_qgroup(uint64_t level, subvolid_t id)
	{
	    return (level << BTRFS_QGROUP_LEVEL_SHIFT) | id;
	}


	uint64_t
	get_level(qgroup_t qgroup)
	{
	    return qgroup >> BTRFS_QGROUP_LEVEL_SHIFT;
	}


	uint64_t
	get_id(qgroup_t qgroup)
	{
	    return qgroup & ((1LLU << BTRFS_QGROUP_LEVEL_SHIFT) - 1);
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
	    ret << get_level(qgroup) << "/" << get_id(qgroup);
	    return ret.str();
	}


	void
	qgroup_create(int fd, qgroup_t qgroup)
	{
	    struct btrfs_ioctl_qgroup_create_args args;
	    memset(&args, 0, sizeof(args));
	    args.create = 1;
	    args.qgroupid = qgroup;

	    if (ioctl(fd, BTRFS_IOC_QGROUP_CREATE, &args) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_QGROUP_CREATE) failed", errno);
	}


	void
	qgroup_destroy(int fd, qgroup_t qgroup)
	{
	    struct btrfs_ioctl_qgroup_create_args args;
	    memset(&args, 0, sizeof(args));
	    args.create = 0;
	    args.qgroupid = qgroup;

	    if (ioctl(fd, BTRFS_IOC_QGROUP_CREATE, &args) < 0)
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

	    if (ioctl(fd, BTRFS_IOC_QGROUP_ASSIGN, &args) < 0)
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

	    if (ioctl(fd, BTRFS_IOC_QGROUP_ASSIGN, &args) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_QGROUP_ASSIGN) failed", errno);
	}


	struct TreeSearchOpts
	{
	    TreeSearchOpts(__u32 type) : min_offset(0), max_offset(-1), min_type(type), max_type(type) {}

	    __u64 min_offset;
	    __u64 max_offset;

	    __u32 min_type;
	    __u32 max_type;

	    std::function<void(const struct btrfs_ioctl_search_args& args,
			       const struct btrfs_ioctl_search_header& sh)> callback;
	};


	/*
	 * Wrapper for ioctl(BTRFS_IOC_TREE_SEARCH). Calls callback of
	 * tree_search_opts for every found item.  In contrast to the bare
	 * ioctl the wrapper ensures that the min and max values in
	 * tree_search_opts are satisfied.  Returns the number of times the
	 * callback was called.
	 */
	size_t
	qgroups_tree_search(int fd, const TreeSearchOpts& tree_search_opts)
	{
	    struct btrfs_ioctl_search_args args;
	    memset(&args, 0, sizeof(args));

	    struct btrfs_ioctl_search_key* sk = &args.key;
	    sk->tree_id = BTRFS_QUOTA_TREE_OBJECTID;
	    sk->min_objectid = 0;
	    sk->max_objectid = BTRFS_LAST_FREE_OBJECTID;
	    sk->min_offset = tree_search_opts.min_offset;
	    sk->max_offset = tree_search_opts.max_offset;
	    sk->min_transid = 0;
	    sk->max_transid = (u64)(-1);
	    sk->min_type = tree_search_opts.min_type;
	    sk->max_type = tree_search_opts.max_type;
	    sk->nr_items = 4096;

	    size_t n = 0;

	    while (true)
	    {
		if (ioctl(fd, BTRFS_IOC_TREE_SEARCH, &args) < 0)
		    throw runtime_error_with_errno("ioctl(BTRFS_IOC_TREE_SEARCH) failed", errno);

		if (sk->nr_items == 0)
		    break;

		u64 off = 0;

		for (unsigned int i = 0; i < sk->nr_items; ++i)
		{
		    struct btrfs_ioctl_search_header* sh = (struct btrfs_ioctl_search_header*)(args.buf + off);

		    if (sh->offset >= tree_search_opts.min_offset && sh->offset <= tree_search_opts.max_offset &&
			sh->type >= tree_search_opts.min_type && sh->type <= tree_search_opts.max_type)
		    {
			tree_search_opts.callback(args, *sh);
			++n;
		    }

		    off += sizeof(*sh) + sh->len;

		    sk->min_type = sh->type;
		    sk->min_objectid = sh->objectid;
		    sk->min_offset = sh->offset;
		}

		sk->nr_items = 4096;

		if (sk->min_offset < (u64)(-1))
		    sk->min_offset++;
		else
		    break;
	    }

	    return n;
	}


	qgroup_t
	qgroup_find_free(int fd, uint64_t level)
	{
	    vector<qgroup_t> qgroups;

	    TreeSearchOpts tree_search_opts(BTRFS_QGROUP_INFO_KEY);
	    tree_search_opts.min_offset = calc_qgroup(level, 0);
	    tree_search_opts.max_offset = calc_qgroup(level, (1LLU << BTRFS_QGROUP_LEVEL_SHIFT) - 1);
	    tree_search_opts.callback = [&qgroups](const struct btrfs_ioctl_search_args& args,
						   const struct btrfs_ioctl_search_header& sh)
	    {
		qgroups.push_back(sh.offset);
	    };

	    qgroups_tree_search(fd, tree_search_opts);

	    if (qgroups.empty() || get_id(qgroups.front()) != 0)
		return calc_qgroup(level, 0);

	    sort(qgroups.begin(), qgroups.end());

	    vector<qgroup_t>::const_iterator it = adjacent_find(qgroups.begin(), qgroups.end(),
		[](qgroup_t a, qgroup_t b) { return get_id(a) + 1 < get_id(b); });

	    if (it == qgroups.end())
		--it;

	    return calc_qgroup(level, get_id(*it) + 1);
	}


	vector<qgroup_t>
	qgroup_query_children(int fd, qgroup_t parent)
	{
	    vector<qgroup_t> ret;

	    TreeSearchOpts tree_search_opts(BTRFS_QGROUP_RELATION_KEY);
	    tree_search_opts.min_offset = tree_search_opts.max_offset = parent;
	    tree_search_opts.callback = [&ret](const struct btrfs_ioctl_search_args& args,
					       const struct btrfs_ioctl_search_header& sh)
	    {
		ret.push_back(sh.objectid);
	    };

	    qgroups_tree_search(fd, tree_search_opts);

	    return ret;
	}


	QGroupUsage
	qgroup_query_usage(int fd, qgroup_t qgroup)
	{
	    QGroupUsage qgroup_usage;

	    TreeSearchOpts tree_search_opts(BTRFS_QGROUP_INFO_KEY);
	    tree_search_opts.min_offset = tree_search_opts.max_offset = qgroup;
	    tree_search_opts.callback = [&qgroup_usage](const struct btrfs_ioctl_search_args& args,
							const struct btrfs_ioctl_search_header& sh)
	    {
		struct btrfs_qgroup_info_item info;
		memcpy(&info, (struct btrfs_qgroup_info_item*)(args.buf + sizeof(sh)), sizeof(info));

		qgroup_usage.referenced = le64_to_cpu(info.referenced);
		qgroup_usage.referenced_compressed = le64_to_cpu(info.referenced_compressed);
		qgroup_usage.exclusive = le64_to_cpu(info.exclusive);
		qgroup_usage.exclusive_compressed = le64_to_cpu(info.exclusive_compressed);
	    };

	    int n = qgroups_tree_search(fd, tree_search_opts);

	    if (n == 0)
		throw std::runtime_error("qgroup info not found");
	    else if (n > 1)
		throw std::runtime_error("several qgroups found");

	    return qgroup_usage;
	}

#endif


	void
	sync(int fd)
	{
#ifdef HAVE_LIBBTRFSUTIL
	    enum btrfs_util_error err;

	    err = btrfs_util_sync_fd(fd);
	    if (err)
		throw runtime_error_with_errno("(btrfs_util_sync_fd() failed", errno);
#else
	    if (ioctl(fd, BTRFS_IOC_SYNC) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_SYNC) failed", errno);
#endif
	}


	Uuid
	get_uuid(int fd)
	{
	    static_assert(BTRFS_UUID_SIZE == 16, "unexpected value of BTRFS_UUID_SIZE");

	    struct btrfs_ioctl_fs_info_args fs_info_args;
	    if (ioctl(fd, BTRFS_IOC_FS_INFO, &fs_info_args) < 0)
		throw runtime_error_with_errno("ioctl(BTRFS_IOC_FS_INFO) failed", errno);

	    Uuid uuid;
	    std::copy(std::begin(fs_info_args.fsid), std::end(fs_info_args.fsid), std::begin(uuid.value));
	    return uuid;
	}


	Uuid
	get_uuid(const string& path)
	{
	    int fd = open(path.c_str(), O_RDONLY);
	    if (fd < 0)
		throw runtime_error_with_errno("open failed", errno);

	    FdCloser fd_closer(fd);

	    return BtrfsUtils::get_uuid(fd);
	}

    }

}
