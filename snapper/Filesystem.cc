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
#include <sys/mount.h>
#include <errno.h>
#include <unistd.h>
#include <mntent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <boost/algorithm/string.hpp>

#include "snapper/Log.h"
#include "snapper/Filesystem.h"
#include "snapper/Snapper.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Regex.h"
#include "config.h"


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


namespace snapper
{

    vector<string>
    filter_mount_options(const vector<string>& options)
    {
	static const char* ign_opt[] = {
	    "ro", "rw",
	    "exec", "noexec", "suid", "nosuid", "dev", "nodev",
	    "atime", "noatime", "diratime", "nodiratime",
	    "relatime", "norelatime", "strictatime", "nostrictatime"
	};

	vector<string> ret = options;

	for (size_t i = 0; i < lengthof(ign_opt); ++i)
	    ret.erase(remove(ret.begin(), ret.end(), ign_opt[i]), ret.end());

	return ret;
    }


    bool
    mount(const string& device, int fd, const string& mount_type, const vector<string>& options)
    {
	unsigned long mount_flags = MS_RDONLY | MS_NOEXEC | MS_NOSUID | MS_NODEV |
	    MS_NOATIME | MS_NODIRATIME;

	string mount_data = boost::join(options, ",");

	int r1 = fchdir(fd);
	if (r1 != 0)
	{
	    y2err("fchdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    return false;
	}

	int r2 = ::mount(device.c_str(), ".", mount_type.c_str(), mount_flags, mount_data.c_str());
	if (r2 != 0)
	{
	    y2err("mount failed errno:" << errno << " (" << stringerror(errno) << ")");
	    chdir("/");
	    return false;
	}

	chdir("/");
	return true;
    }


    bool
    umount(int fd, const string& mount_point)
    {
	int r1 = fchdir(fd);
	if (r1 != 0)
	{
	    y2err("fchdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    return false;
	}

#ifdef UMOUNT_NOFOLLOW
	int r2 = ::umount2(mount_point.c_str(), UMOUNT_NOFOLLOW);
#else
	int r2 = ::umount2(mount_point.c_str(), 0);
#endif
	if (r2 != 0)
	{
	    y2err("umount failed errno:" << errno << " (" << stringerror(errno) << ")");
	    chdir("/");
	    return false;
	}

	chdir("/");
	return true;
    }


    Filesystem*
    Filesystem::create(const string& fstype, const string& subvolume)
    {
	typedef Filesystem* (*func_t)(const string& fstype, const string& subvolume);

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
	NULL };

	for (const func_t* func = funcs; *func != NULL; ++func)
	{
	    Filesystem* fs = (*func)(fstype, subvolume);
	    if (fs)
		return fs;
	}

	y2err("do not know about fstype '" << fstype << "'");
	throw InvalidConfigException();
    }


    SDir
    Filesystem::openSubvolumeDir() const
    {
	SDir subvolume_dir(subvolume);

	return subvolume_dir;
    }


    SDir
    Filesystem::openInfoDir(unsigned int num) const
    {
	SDir infos_dir = openInfosDir();
	SDir info_dir(infos_dir, decString(num));

	return info_dir;
    }


#ifdef ENABLE_BTRFS
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

	if (stat.st_uid != 0 || stat.st_gid != 0)
	{
	    y2err("owner/group of .snapshots wrong");
	    throw IOErrorException();
	}

	if (stat.st_mode & S_IWOTH)
	{
	    y2err("permissions of .snapshots wrong");
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
    // ENABLE_BTRFS
#endif


#ifdef ENABLE_EXT4
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

	mount_options = filter_mount_options(mtab_data.options);
	mount_options.push_back("loop");
	mount_options.push_back("noload");
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
	    y2err("mkdir failed errno:" << errno << " (" << stringerror(errno) << ")");
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
	    y2err("mkdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw CreateConfigFailedException("mkdir failed");
	}
    }


    void
    Ext4::deleteConfig() const
    {
	int r1 = rmdir((subvolume + "/.snapshots/.info").c_str());
	if (r1 != 0)
	{
	    y2err("rmdir failed errno:" << errno << " (" << stringerror(errno) << ")");
	    throw DeleteConfigFailedException("rmdir failed");
	}

	int r2 = rmdir((subvolume + "/.snapshots").c_str());
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
	return (subvolume == "/" ? "" : subvolume) + "/.snapshots/" + decString(num);
    }


    SDir
    Ext4::openInfosDir() const
    {
	// TODO
    }


    SDir
    Ext4::openSnapshotDir(unsigned int num) const
    {
	// TODO
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

	SystemCmd cmd1(CHSNAPBIN " -n " + quote(snapshotFile(num)));
	if (cmd1.retcode() != 0)
	    throw UmountSnapshotFailedException();

	rmdir(snapshotDir(num).c_str());
    }


    bool
    Ext4::checkSnapshot(unsigned int num) const
    {
	struct stat stat;
	int r1 = ::stat(snapshotFile(num).c_str(), &stat);
	return r1 == 0 && S_ISREG(stat.st_mode);
    }
    // ENABLE_EXT4
#endif


#ifdef ENABLE_LVM
    Filesystem*
    Lvm::create(const string& fstype, const string& subvolume)
    {
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

	mount_options = filter_mount_options(mtab_data.options);
	if (mount_type == "xfs")
	    mount_options.push_back("nouuid");
    }


    void
    Lvm::createConfig() const
    {
	SDir subvolume_dir = openSubvolumeDir();

	int r1 = subvolume_dir.mkdir(".snapshots", 0750);
	if (r1 != 0 && errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw CreateConfigFailedException("mkdir failed");
	}
    }


    void
    Lvm::deleteConfig() const
    {
	SDir subvolume_dir = openSubvolumeDir();

	int r1 = subvolume_dir.unlink(".snapshots", AT_REMOVEDIR);
	if (r1 != 0)
	{
	    y2err("rmdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw DeleteConfigFailedException("rmdir failed");
	}
    }


    string
    Lvm::snapshotDir(unsigned int num) const
    {
	return (subvolume == "/" ? "" : subvolume) + "/.snapshots/" + decString(num) +
	    "/snapshot";
    }


    SDir
    Lvm::openInfosDir() const
    {
	SDir subvolume_dir = openSubvolumeDir();
	SDir infos_dir(subvolume_dir, ".snapshots");

	struct stat stat;
	if (infos_dir.stat(&stat) != 0)
	{
	    throw IOErrorException();
	}

	if (stat.st_uid != 0 || stat.st_gid != 0)
	{
	    y2err("owner/group of .snapshots wrong");
	    throw IOErrorException();
	}

	if (stat.st_mode & S_IWOTH)
	{
	    y2err("permissions of .snapshots wrong");
	    throw IOErrorException();
	}

	return infos_dir;
    }


    SDir
    Lvm::openSnapshotDir(unsigned int num) const
    {
	SDir info_dir = openInfoDir(num);
	SDir snapshot_dir(info_dir, "snapshot");

	return snapshot_dir;
    }


    string
    Lvm::snapshotLvName(unsigned int num) const
    {
	return lv_name + "-snapshot" + decString(num);
    }


    void
    Lvm::createSnapshot(unsigned int num) const
    {
	{
	    // TODO looks like a bug that this is needed (with ext4)
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 14)
	    SDir subvolume_dir = openSubvolumeDir();
	    syncfs(subvolume_dir.fd());
#else
	    sync();
#endif
	}

	SystemCmd cmd(LVCREATE " --snapshot --name " + quote(snapshotLvName(num)) + " " +
		      quote(vg_name + "/" + lv_name));
	if (cmd.retcode() != 0)
	    throw CreateSnapshotFailedException();

	SDir info_dir = openInfoDir(num);
	int r1 = info_dir.mkdir("snapshot", 0755);
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

	SDir info_dir = openInfoDir(num);
	info_dir.unlink("snapshot", AT_REMOVEDIR);

	SDir infos_dir = openInfosDir();
	infos_dir.unlink(decString(num), AT_REMOVEDIR);
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

	SDir snapshot_dir = openSnapshotDir(num);

	if (!mount(getDevice(num), snapshot_dir.fd(), mount_type, mount_options))
	    throw MountSnapshotFailedException();
    }


    void
    Lvm::umountSnapshot(unsigned int num) const
    {
	if (!isSnapshotMounted(num))
	    return;

	SDir info_dir = openInfoDir(num);

	if (!umount(info_dir.fd(), "snapshot"))
	    throw UmountSnapshotFailedException();
    }


    bool
    Lvm::checkSnapshot(unsigned int num) const
    {
	struct stat stat;
	int r1 = ::stat(getDevice(num).c_str(), &stat);
	return r1 == 0 && S_ISBLK(stat.st_mode);
    }


    bool
    Lvm::detectLvmNames(const MtabData& mtab_data)
    {
	Regex rx("^/dev/mapper/(.+[^-])-([^-].+)$");
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
    // ENABLE_LVM
#endif

}
