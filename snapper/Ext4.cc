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

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <boost/algorithm/string.hpp>

#include "snapper/LoggerImpl.h"
#include "snapper/Filesystem.h"
#include "snapper/Ext4.h"
#include "snapper/Snapper.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"


namespace snapper
{

    std::unique_ptr<Filesystem>
    Ext4::create(const string& fstype, const string& subvolume, const string& root_prefix)
    {
	if (fstype == "ext4")
	    return std::make_unique<Ext4>(subvolume, root_prefix);

	return nullptr;
    }


    Ext4::Ext4(const string& subvolume, const string& root_prefix)
	: Filesystem(subvolume, root_prefix)
    {
	if (access(CHSNAP_BIN, X_OK) != 0)
	{
	    throw ProgramNotInstalledException(CHSNAP_BIN " not installed");
	}

	if (access(CHATTR_BIN, X_OK) != 0)
	{
	    throw ProgramNotInstalledException(CHATTR_BIN " not installed");
	}

	bool found = false;
	MtabData mtab_data;

	if (!getMtabData(subvolume, found, mtab_data))
	    throw InvalidConfigException();

	if (!found)
	{
	    y2err("filesystem not mounted");
	    throw InvalidConfigException();
	}

	mount_options = filter_mount_options(mtab_data.options);
	mount_options.push_back("loop");
	mount_options.push_back("noload");
    }


    void
    Ext4::createConfig() const
    {
	int r1 = mkdir((subvolume + "/" SNAPSHOTS_NAME).c_str(), 0700);
	if (r1 == 0)
	{
	    SystemCmd cmd1({ CHATTR_BIN, "+x", subvolume + "/" SNAPSHOTS_NAME });
	    if (cmd1.retcode() != 0)
		throw CreateConfigFailedException("chattr failed");
	}
	else if (errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw CreateConfigFailedException("mkdir failed");
	}

	int r2 = mkdir((subvolume + "/" SNAPSHOTS_NAME "/.info").c_str(), 0700);
	if (r2 == 0)
	{
	    SystemCmd cmd2({ CHATTR_BIN, "-x", subvolume + "/" SNAPSHOTS_NAME "/.info" });
	    if (cmd2.retcode() != 0)
		throw CreateConfigFailedException("chattr failed");
	}
	else if (errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw CreateConfigFailedException("mkdir failed");
	}
    }


    void
    Ext4::deleteConfig() const
    {
	int r1 = rmdir((subvolume + "/" SNAPSHOTS_NAME "/.info").c_str());
	if (r1 != 0)
	{
	    y2err("rmdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw DeleteConfigFailedException("rmdir failed");
	}

	int r2 = rmdir((subvolume + "/" SNAPSHOTS_NAME).c_str());
	if (r2 != 0)
	{
	    y2err("rmdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw DeleteConfigFailedException("rmdir failed");
	}
    }


    string
    Ext4::snapshotDir(unsigned int num) const
    {
	return subvolume + "@" + decString(num);
    }


    string
    Ext4::snapshotFile(unsigned int num) const
    {
	return (subvolume == "/" ? "" : subvolume) + "/" SNAPSHOTS_NAME "/" + decString(num);
    }


    SDir
    Ext4::openInfosDir() const
    {
	// TODO
	SDir not_there("/dev/null");
	return not_there;
    }


    SDir
    Ext4::openSnapshotDir(unsigned int num) const
    {
	// TODO
	SDir not_there("/dev/null");
	return not_there;
    }


    void
    Ext4::createSnapshot(unsigned int num, unsigned int num_parent, bool read_only, bool quota,
			 bool empty) const
    {
	if (num_parent != 0 || !read_only)
	    throw std::logic_error("not implemented");

	SystemCmd cmd1({ TOUCH_BIN, snapshotFile(num) });
	if (cmd1.retcode() != 0)
	    throw CreateSnapshotFailedException();

	SystemCmd cmd2({ CHSNAP_BIN, "+S", snapshotFile(num) });
	if (cmd2.retcode() != 0)
	    throw CreateSnapshotFailedException();
    }


    void
    Ext4::deleteSnapshot(unsigned int num) const
    {
	SystemCmd cmd({ CHSNAP_BIN, "-S", snapshotFile(num) });
	if (cmd.retcode() != 0)
	    throw DeleteSnapshotFailedException();
    }


    bool
    Ext4::isSnapshotMounted(unsigned int num) const
    {
	bool mounted = false;
	MtabData mtab_data;

	if (!getMtabData(snapshotDir(num), mounted, mtab_data))
	    throw IsSnapshotMountedFailedException();

	return mounted;
    }


    void
    Ext4::mountSnapshot(unsigned int num) const
    {
	if (isSnapshotMounted(num))
	    return;

	SystemCmd cmd1({ CHSNAP_BIN, "+n", snapshotFile(num) });
	if (cmd1.retcode() != 0)
	    throw MountSnapshotFailedException();

	int r1 = mkdir(snapshotDir(num).c_str(), 0755);
	if (r1 != 0 && errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw MountSnapshotFailedException();
	}

	// if (!mount(snapshotFile(num), snapshotDir(num), "ext4", mount_options))
	// throw MountSnapshotFailedException();
    }


    void
    Ext4::umountSnapshot(unsigned int num) const
    {
	if (!isSnapshotMounted(num))
	    return;

	// if (!umount(snapshotDir(num)))
	// throw UmountSnapshotFailedException();

	SystemCmd cmd1({ CHSNAP_BIN, "-n", snapshotFile(num) });
	if (cmd1.retcode() != 0)
	    throw UmountSnapshotFailedException();

	rmdir(snapshotDir(num).c_str());
    }


    bool
    Ext4::isSnapshotReadOnly(unsigned int num) const
    {
	// TODO

	return true;
    }


    void
    Ext4::setSnapshotReadOnly(unsigned int num, bool read_only, Plugins::Report& report) const
    {
	// TODO
    }


    bool
    Ext4::checkSnapshot(unsigned int num) const
    {
	struct stat stat;
	int r1 = ::stat(snapshotFile(num).c_str(), &stat);
	return r1 == 0 && S_ISREG(stat.st_mode);
    }

}
