/*
 * Copyright (c) 2024 SUSE LLC
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
#include <linux/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "snapper/LoggerImpl.h"
#include "snapper/AppUtil.h"
#include "snapper/BcachefsUtils.h"


struct bch_ioctl_subvolume {
	__u32			flags;
	__u32			dirfd;
	__u16			mode;
	__u16			pad[3];
	__u64			dst_ptr;
	__u64			src_ptr;
};

#define BCH_IOCTL_SUBVOLUME_CREATE	_IOW(0xbc, 16, struct bch_ioctl_subvolume)
#define BCH_IOCTL_SUBVOLUME_DESTROY	_IOW(0xbc, 17, struct bch_ioctl_subvolume)

#define BCH_SUBVOL_SNAPSHOT_CREATE	(1U << 0)
#define BCH_SUBVOL_SNAPSHOT_RO		(1U << 1)


namespace snapper
{

    namespace BcachefsUtils
    {

	bool
        is_subvolume(const struct stat& stat)
        {
            return true;	// TODO
        }


	bool
	is_subvolume_read_only(int fd)
	{
	    return false;	// TODO
	}


	void
	set_subvolume_read_only(int fd, bool read_only)
	{
	    throw std::runtime_error("set_subvolume_read_only not implemented");	// TODO
	}


	void
	create_subvolume(int fddst, const string& name)
	{
	    struct bch_ioctl_subvolume args;
	    memset(&args, 0, sizeof(args));

	    args.flags = 0;
	    args.dirfd = (__u32) fddst;
	    args.mode = 0777;
	    args.dst_ptr = (__u64) name.c_str();

	    if (ioctl(fddst, BCH_IOCTL_SUBVOLUME_CREATE, &args) < 0)
		throw runtime_error_with_errno("ioctl(BCH_IOCTL_SUBVOLUME_CREATE) failed", errno);
	}


	void
	create_snapshot(int fd, const string& subvolume, int fddst, const string& name, bool read_only)
	{
	    struct bch_ioctl_subvolume args;
	    memset(&args, 0, sizeof(args));

	    args.flags = BCH_SUBVOL_SNAPSHOT_CREATE;
	    if (read_only)
		args.flags |= BCH_SUBVOL_SNAPSHOT_RO;
	    args.dirfd = (__u32) fddst;
	    args.mode = 0777;
	    args.dst_ptr = (__u64) name.c_str();
	    args.src_ptr = (__u64) subvolume.c_str();	// TODO use fd instead of subvolume

	    if (ioctl(fddst, BCH_IOCTL_SUBVOLUME_CREATE, &args) < 0)
		throw runtime_error_with_errno("ioctl(BCH_IOCTL_SUBVOLUME_CREATE) failed", errno);
	}


	void
	delete_subvolume(int fd, const string& name)
	{
	    struct bch_ioctl_subvolume args;
	    memset(&args, 0, sizeof(args));

	    args.flags = 0;
	    args.dirfd = (__u32) fd;
	    args.dst_ptr = (__u64) name.c_str();

	    if (ioctl(fd, BCH_IOCTL_SUBVOLUME_DESTROY, &args) < 0)
		throw runtime_error_with_errno("ioctl(BCH_IOCTL_SUBVOLUME_DESTROY) failed", errno);
	}

    }

}
