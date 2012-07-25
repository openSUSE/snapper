/*
 * Copyright (c) [2011-2012] Novell, Inc.
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


#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <boost/algorithm/string.hpp>

#include "snapper/Log.h"
#include "snapper/Filesystem.h"
#include "snapper/Snapper.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Regex.h"


namespace snapper
{

    bool
    mount(const string& device, const string& mount_point, const string& mount_type,
	  const vector<string>& old_options, const vector<string>& new_options)
    {
	vector<string> options = old_options;
	options.erase(remove(options.begin(), options.end(), "rw"), options.end());
	options.insert(options.end(), new_options.begin(), new_options.end());

	string cmd_line = MOUNTBIN " -t " + mount_type + " --read-only";

	if (!options.empty())
	    cmd_line += " -o " + boost::join(options, ",");

	cmd_line += " " + quote(device) + " " + quote(mount_point);

	SystemCmd cmd(cmd_line);
	return cmd.retcode() == 0;
    }


    bool
    umount(const string& mount_point)
    {
	SystemCmd cmd(UMOUNTBIN " " + quote(mount_point));
	return cmd.retcode() == 0;
    }


    Filesystem*
    Filesystem::create(const string& fstype, const string& subvolume)
    {
	typedef Filesystem* (*func_t)(const string& fstype, const string& subvolume);

	static const func_t funcs[] = { &Btrfs::create, &Ext4::create, &Lvm::create, NULL };

	for (const func_t* func = funcs; func != NULL; ++func)
	{
	    Filesystem* fs = (*func)(fstype, subvolume);
	    if (fs)
		return fs;
	}

	y2err("do not know about fstype '" << fstype << "'");
	throw InvalidConfigException();
    }


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
	if (access(BTRFSBIN, X_OK) != 0)
	{
	    throw ProgramNotInstalledException(BTRFSBIN " not installed");
	}
    }


    void
    Btrfs::createConfig() const
    {
	SystemCmd cmd2(BTRFSBIN " subvolume create " + quote(subvolume + "/.snapshots"));
	if (cmd2.retcode() != 0)
	    throw CreateConfigFailedException("creating btrfs snapshot failed");
    }


    void
    Btrfs::deleteConfig() const
    {
	SystemCmd cmd2(BTRFSBIN " subvolume delete " + quote(subvolume + "/.snapshots"));
	if (cmd2.retcode() != 0)
	    throw DeleteConfigFailedException("deleting btrfs snapshot failed");
    }


    string
    Btrfs::infosDir() const
    {
	return (subvolume == "/" ? "" : subvolume) + "/.snapshots";
    }


    string
    Btrfs::snapshotDir(unsigned int num) const
    {
	return (subvolume == "/" ? "" : subvolume) + "/.snapshots/" + decString(num) +
	    "/snapshot";
    }


    void
    Btrfs::createSnapshot(unsigned int num) const
    {
	SystemCmd cmd(BTRFSBIN " subvolume snapshot -r " + quote(subvolume) + " " +
		      quote(snapshotDir(num)));
	if (cmd.retcode() != 0)
	    throw CreateSnapshotFailedException();
    }


    void
    Btrfs::deleteSnapshot(unsigned int num) const
    {
	SystemCmd cmd(BTRFSBIN " subvolume delete " + quote(snapshotDir(num)));
	if (cmd.retcode() != 0)
	    throw DeleteSnapshotFailedException();
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
	return checkDir(snapshotDir(num));
    }


    Filesystem*
    Ext4::create(const string& fstype, const string& subvolume)
    {
	if (fstype == "ext4")
	    return new Ext4(subvolume);

	return NULL;
    }


    Ext4::Ext4(const string& subvolume)
	: Filesystem(subvolume)
    {
	if (access(CHSNAPBIN, X_OK) != 0)
	{
	    throw ProgramNotInstalledException(CHSNAPBIN " not installed");
	}

	if (access(CHATTRBIN, X_OK) != 0)
	{
	    throw ProgramNotInstalledException(CHATTRBIN " not installed");
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

	mount_options = mtab_data.options;
    }


    void
    Ext4::createConfig() const
    {
	int r1 = mkdir((subvolume + "/.snapshots").c_str(), 0700);
	if (r1 == 0)
	{
	    SystemCmd cmd1(CHATTRBIN " +x " + quote(subvolume + "/.snapshots"));
	    if (cmd1.retcode() != 0)
		throw CreateConfigFailedException("chattr failed");
	}
	else if (errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw CreateConfigFailedException("mkdir failed");
	}

	int r2 = mkdir((subvolume + "/.snapshots/.info").c_str(), 0700);
	if (r2 == 0)
	{
	    SystemCmd cmd2(CHATTRBIN " -x " + quote(subvolume + "/.snapshots/.info"));
	    if (cmd2.retcode() != 0)
		throw CreateConfigFailedException("chattr failed");
	}
	else if (errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw CreateConfigFailedException("mkdir failed");
	}
    }


    void
    Ext4::deleteConfig() const
    {
	int r1 = rmdir((subvolume + "/.snapshots/.info").c_str());
	if (r1 != 0)
	{
	    y2err("rmdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw DeleteConfigFailedException("rmdir failed");
	}

	int r2 = rmdir((subvolume + "/.snapshots").c_str());
	if (r2 != 0)
	{
	    y2err("rmdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw DeleteConfigFailedException("rmdir failed");
	}
    }


    string
    Ext4::infosDir() const
    {
	return (subvolume == "/" ? "" : subvolume) + "/.snapshots/.info";
    }


    string
    Ext4::snapshotDir(unsigned int num) const
    {
	return subvolume + "@" + decString(num);
    }


    string
    Ext4::snapshotFile(unsigned int num) const
    {
	return (subvolume == "/" ? "" : subvolume) + "/.snapshots/" + decString(num);
    }


    void
    Ext4::createSnapshot(unsigned int num) const
    {
	SystemCmd cmd1(TOUCHBIN " " + quote(snapshotFile(num)));
	if (cmd1.retcode() != 0)
	    throw CreateSnapshotFailedException();

	SystemCmd cmd2(CHSNAPBIN " +S " + quote(snapshotFile(num)));
	if (cmd2.retcode() != 0)
	    throw CreateSnapshotFailedException();
    }


    void
    Ext4::deleteSnapshot(unsigned int num) const
    {
	SystemCmd cmd(CHSNAPBIN " -S " + quote(snapshotFile(num)));
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

	SystemCmd cmd1(CHSNAPBIN " +n " + quote(snapshotFile(num)));
	if (cmd1.retcode() != 0)
	    throw MountSnapshotFailedException();

	int r1 = mkdir(snapshotDir(num).c_str(), 0755);
	if (r1 != 0 && errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw MountSnapshotFailedException();
	}

	vector<string> options;
	options.push_back("noatime");
	options.push_back("loop");
	options.push_back("noload");

	if (!mount(snapshotFile(num), snapshotDir(num), "ext4", mount_options, options))
	    throw MountSnapshotFailedException();
    }


    void
    Ext4::umountSnapshot(unsigned int num) const
    {
	if (!isSnapshotMounted(num))
	    return;

	if (!umount(snapshotDir(num)))
	    throw UmountSnapshotFailedException();

	SystemCmd cmd1(CHSNAPBIN " -n " + quote(snapshotFile(num)));
	if (cmd1.retcode() != 0)
	    throw UmountSnapshotFailedException();

	rmdir(snapshotDir(num).c_str());
    }


    bool
    Ext4::checkSnapshot(unsigned int num) const
    {
	return checkNormalFile(snapshotFile(num));
    }


    Filesystem*
    Lvm::create(const string& fstype, const string& subvolume)
    {
	if (fstype == "lvm")
	    return new Lvm(subvolume, "auto");

	Regex rx("^lvm\\(([_a-z0-9]+)\\)$");
	if (rx.match(fstype))
	    return new Lvm(subvolume, rx.cap(1));

	return NULL;
    }


    Lvm::Lvm(const string& subvolume, const string& mount_type)
	: Filesystem(subvolume), mount_type(mount_type)
    {
	if (access(LVCREATE, X_OK) != 0)
	{
	    throw ProgramNotInstalledException(LVCREATE " not installed");
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

	if (!detectLvmNames(mtab_data))
	    throw InvalidConfigException();

	mount_options = mtab_data.options;
    }


    void
    Lvm::createConfig() const
    {
	int r1 = mkdir((subvolume + "/.snapshots").c_str(), 0700);
	if (r1 != 0 && errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw CreateConfigFailedException("mkdir failed");
	}
    }


    void
    Lvm::deleteConfig() const
    {
	int r1 = rmdir((subvolume + "/.snapshots").c_str());
	if (r1 != 0)
	{
	    y2err("rmdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw DeleteConfigFailedException("rmdir failed");
	}
    }


    string
    Lvm::infosDir() const
    {
	return (subvolume == "/" ? "" : subvolume) + "/.snapshots";
    }


    string
    Lvm::snapshotDir(unsigned int num) const
    {
	return (subvolume == "/" ? "" : subvolume) + "/.snapshots/" + decString(num) +
	    "/snapshot";
    }


    string
    Lvm::snapshotLvName(unsigned int num) const
    {
	return lv_name + "-snapshot" + decString(num);
    }


    void
    Lvm::createSnapshot(unsigned int num) const
    {
	sync();			// TODO looks like a bug that this is needed (with ext4)

	SystemCmd cmd(LVCREATE " --snapshot --name " + quote(snapshotLvName(num)) + " " +
		      quote(vg_name + "/" + lv_name));
	if (cmd.retcode() != 0)
	    throw CreateSnapshotFailedException();

	int r1 = mkdir(snapshotDir(num).c_str(), 0700);
	if (r1 != 0 && errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw CreateSnapshotFailedException();
	}
    }


    void
    Lvm::deleteSnapshot(unsigned int num) const
    {
	SystemCmd cmd(LVREMOVE " --force " + quote(vg_name + "/" + snapshotLvName(num)));
	if (cmd.retcode() != 0)
	    throw DeleteSnapshotFailedException();

	rmdir(snapshotDir(num).c_str());
    }


    bool
    Lvm::isSnapshotMounted(unsigned int num) const
    {
	bool mounted = false;
	MtabData mtab_data;

	if (!getMtabData(snapshotDir(num), mounted, mtab_data))
	    throw IsSnapshotMountedFailedException();

	return mounted;
    }


    void
    Lvm::mountSnapshot(unsigned int num) const
    {
	if (isSnapshotMounted(num))
	    return;

	vector<string> options;
	options.push_back("noatime");
	if (mount_type == "xfs")
	    options.push_back("nouuid");

	if (!mount(getDevice(num), snapshotDir(num), mount_type, mount_options, options))
	    throw MountSnapshotFailedException();
    }


    void
    Lvm::umountSnapshot(unsigned int num) const
    {
	if (!isSnapshotMounted(num))
	    return;

	if (!umount(snapshotDir(num)))
	    throw UmountSnapshotFailedException();
    }


    bool
    Lvm::checkSnapshot(unsigned int num) const
    {
	return checkAnything(getDevice(num));
    }


    bool
    Lvm::detectLvmNames(const MtabData& mtab_data)
    {
	Regex rx("^/dev/mapper/([^-]+)-([^-]+)$");
	if (rx.match(mtab_data.device))
	{
	    vg_name = boost::replace_all_copy(rx.cap(1), "--", "-");
	    lv_name = boost::replace_all_copy(rx.cap(2), "--", "-");
	    return true;
	}

	y2err("could not detect lvm names from '" << mtab_data.device << "'");
	return false;
    }


    string
    Lvm::getDevice(unsigned int num) const
    {
	return "/dev/mapper/" + boost::replace_all_copy(vg_name, "-", "--") + "-" +
	    boost::replace_all_copy(snapshotLvName(num), "-", "--");
    }

}
