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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "snapper/LoggerImpl.h"
#include "snapper/Bcachefs.h"
#include "snapper/BcachefsUtils.h"
#include "snapper/File.h"
#include "snapper/Snapper.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SnapperDefines.h"

#include "snapper/Acls.h"
#include "snapper/Exception.h"
#ifdef ENABLE_SELINUX
#include "snapper/Selinux.h"
#endif


namespace snapper
{
    using namespace std;

    using namespace BcachefsUtils;


    std::unique_ptr<Filesystem>
    Bcachefs::create(const string& fstype, const string& subvolume, const string& root_prefix)
    {
	if (fstype == "bcachefs")
	    return std::make_unique<Bcachefs>(subvolume, root_prefix);

	return nullptr;
    }


    Bcachefs::Bcachefs(const string& subvolume, const string& root_prefix)
	: Filesystem(subvolume, root_prefix)
    {
    }


    void
    Bcachefs::createConfig() const
    {
	SDir subvolume_dir = openSubvolumeDir();

	try
	{
	    create_subvolume(subvolume_dir.fd(), SNAPSHOTS_NAME);
	}
	catch (const runtime_error_with_errno& e)
	{
	    y2err("create subvolume failed, " << e.what());

	    switch (e.error_number)
	    {
		case EEXIST:
		    SN_THROW(CreateConfigFailedException("creating bcachefs subvolume .snapshots failed "
							 "since it already exists"));
		    break;

		default:
		    SN_THROW(CreateConfigFailedException("creating bcachefs subvolume .snapshots failed"));
	    }
	}

	SFile x(subvolume_dir, SNAPSHOTS_NAME);

#ifdef ENABLE_SELINUX
	try
	{
	    SnapperContexts scontexts;

	    x.fsetfilecon(scontexts.subvolume_context());
	}
	catch (const SelinuxException& e)
	{
	    SN_CAUGHT(e);
	    // fall through intentional
	}
#endif

	struct stat stat;
	if (x.stat(&stat, 0) == 0)
	    x.chmod(stat.st_mode & ~0027, 0);
    }


    void
    Bcachefs::deleteConfig() const
    {
	SDir subvolume_dir = openSubvolumeDir();

	try
	{
	    delete_subvolume(subvolume_dir.fd(), SNAPSHOTS_NAME);
	}
	catch (const runtime_error& e)
	{
	    y2err("delete subvolume failed, " << e.what());
	    SN_THROW(DeleteConfigFailedException("deleting bcachefs snapshot failed"));
	}
    }


    string
    Bcachefs::snapshotDir(unsigned int num) const
    {
	return (subvolume == "/" ? "" : subvolume) + "/" SNAPSHOTS_NAME "/" + decString(num) +
	    "/" SNAPSHOT_NAME;
    }


    SDir
    Bcachefs::openSubvolumeDir() const
    {
	SDir subvolume_dir = Filesystem::openSubvolumeDir();

	struct stat stat;
	if (subvolume_dir.stat(&stat) != 0)
	{
	    SN_THROW(IOErrorException("stat on subvolume directory failed"));
	}

	if (!is_subvolume(stat))
	{
	    SN_THROW(IOErrorException("subvolume is not a bcachefs subvolume"));
	}

	return subvolume_dir;
    }


    SDir
    Bcachefs::openInfosDir() const
    {
	SDir subvolume_dir = openSubvolumeDir();
	SDir infos_dir(subvolume_dir, SNAPSHOTS_NAME);

	struct stat stat;
	if (infos_dir.stat(&stat) != 0)
	{
	    SN_THROW(IOErrorException("stat on info directory failed"));
	}

	if (!is_subvolume(stat))
	{
	    SN_THROW(IOErrorException(".snapshots is not a bcachefs subvolume"));
	}

	if (stat.st_uid != 0)
	{
	    y2err(".snapshots must have owner root");
	    SN_THROW(IOErrorException(".snapshots must have owner root"));
	}

	if (stat.st_gid != 0 && stat.st_mode & S_IWGRP)
	{
	    y2err(".snapshots must have group root or must not be group-writable");
	    SN_THROW(IOErrorException(".snapshots must have group root or must not be group-writable"));
	}

	if (stat.st_mode & S_IWOTH)
	{
	    y2err(".snapshots must not be world-writable");
	    SN_THROW(IOErrorException(".snapshots must not be world-writable"));
	}

	return infos_dir;
    }


    SDir
    Bcachefs::openSnapshotDir(unsigned int num) const
    {
	SDir info_dir = openInfoDir(num);
	SDir snapshot_dir(info_dir, SNAPSHOT_NAME);

	return snapshot_dir;
    }


    void
    Bcachefs::createSnapshot(unsigned int num, unsigned int num_parent, bool read_only, bool quota,
			     bool empty) const
    {
	if (num_parent == 0)
	{
	    SDir subvolume_dir = openSubvolumeDir();
	    SDir info_dir = openInfoDir(num);

	    try
	    {
		if (empty)
		    create_subvolume(info_dir.fd(), SNAPSHOT_NAME);
		else
		    create_snapshot(subvolume_dir.fd(), subvolume, info_dir.fd(), SNAPSHOT_NAME, read_only);
	    }
	    catch (const runtime_error& e)
	    {
		y2err("create snapshot failed, " << e.what());
		SN_THROW(CreateSnapshotFailedException());
	    }
	}
	else
	{
	    SDir snapshot_dir = openSnapshotDir(num_parent);
	    SDir info_dir = openInfoDir(num);

	    try
	    {
		create_snapshot(snapshot_dir.fd(), subvolume, info_dir.fd(), SNAPSHOT_NAME, read_only);
	    }
	    catch (const runtime_error& e)
	    {
		y2err("create snapshot failed, " << e.what());
		SN_THROW(CreateSnapshotFailedException());
	    }
	}
    }


    void
    Bcachefs::deleteSnapshot(unsigned int num) const
    {
	SDir info_dir = openInfoDir(num);

	try
	{
	    delete_subvolume(info_dir.fd(), SNAPSHOT_NAME);
	}
	catch (const runtime_error& e)
	{
	    y2err("delete snapshot failed, " << e.what());
	    SN_THROW(DeleteSnapshotFailedException());
	}
    }


    bool
    Bcachefs::isSnapshotMounted(unsigned int num) const
    {
	return true;
    }


    void
    Bcachefs::mountSnapshot(unsigned int num) const
    {
    }


    void
    Bcachefs::umountSnapshot(unsigned int num) const
    {
    }


    bool
    Bcachefs::isSnapshotReadOnly(unsigned int num) const
    {
	SDir snapshot_dir = openSnapshotDir(num);
	return is_subvolume_read_only(snapshot_dir.fd());
    }


    void
    Bcachefs::setSnapshotReadOnly(unsigned int num, bool read_only) const
    {
	SDir snapshot_dir = openSnapshotDir(num);
	set_subvolume_read_only(snapshot_dir.fd(), read_only);
    }


    bool
    Bcachefs::checkSnapshot(unsigned int num) const
    {
	try
	{
	    SDir info_dir = openInfoDir(num);

	    struct stat stat;
	    int r = info_dir.stat(SNAPSHOT_NAME, &stat, AT_SYMLINK_NOFOLLOW);
	    return r == 0 && is_subvolume(stat);
	}
	catch (const IOErrorException& e)
	{
	    SN_CAUGHT(e);

	    // TODO the openInfoDir above logs an error although when this
	    // function is used from nextNumber the failure is ok

	    return false;
	}
    }

}
