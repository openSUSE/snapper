/*
 * Copyright (c) [2011-2014] Novell, Inc.
 * Copyright (c) [2020-2023] SUSE LLC
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
#include <regex>
#include <boost/algorithm/string.hpp>

#include "snapper/LoggerImpl.h"
#include "snapper/Filesystem.h"
#include "snapper/Lvm.h"
#include "snapper/LvmUtils.h"
#include "snapper/Snapper.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/LvmCache.h"
#ifdef ENABLE_SELINUX
#include "snapper/Selinux.h"
#endif


namespace snapper
{
    using namespace std;


    Filesystem*
    Lvm::create(const string& fstype, const string& subvolume, const string& root_prefix)
    {
	static const regex rx("lvm\\(([_a-z0-9]+)\\)", regex::extended);
	smatch match;

	if (regex_match(fstype, match, rx))
	    return new Lvm(subvolume, root_prefix, match[1]);

	return nullptr;
    }


    Lvm::Lvm(const string& subvolume, const string& root_prefix, const string& mount_type)
	: Filesystem(subvolume, root_prefix), mount_type(mount_type),
	  cache(LvmCache::get_lvm_cache())
    {
	if (access(LVCREATE_BIN, X_OK) != 0)
	{
	    SN_THROW(ProgramNotInstalledException(LVCREATE_BIN " not installed"));
	}

	if (access(LVS_BIN, X_OK) != 0)
	{
	    SN_THROW(ProgramNotInstalledException(LVS_BIN " not installed"));
	}

	if (access(LVCHANGE_BIN, X_OK) != 0)
	{
	    SN_THROW(ProgramNotInstalledException(LVCHANGE_BIN " not installed"));
	}

	bool found = false;
	MtabData mtab_data;

	if (!getMtabData(prepend_root_prefix(root_prefix, subvolume), found, mtab_data))
	    SN_THROW(IOErrorException("filesystem not mounted"));

	if (!found)
	    SN_THROW(IOErrorException("filesystem not mounted"));

	if (!detectThinVolumeNames(mtab_data))
	    SN_THROW(InvalidConfigException());

	mount_options = filter_mount_options(mtab_data.options);
	if (mount_type == "xfs")
	{
	    mount_options.push_back("nouuid");
	    mount_options.push_back("norecovery");
	}
    }


    void
    Lvm::createLvmConfig(const SDir& subvolume_dir, int mode) const
    {
	int r1 = subvolume_dir.mkdir(SNAPSHOTS_NAME, mode);
	if (r1 != 0 && errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    SN_THROW(CreateConfigFailedException("mkdir failed"));
	}
    }


    void
    Lvm::createConfig() const
    {
	int mode = 0750;
	SDir subvolume_dir = openSubvolumeDir();

#ifdef ENABLE_SELINUX
	if (_is_selinux_enabled())
	{
	    SelinuxLabelHandle* selabel_handle = SelinuxLabelHandle::get_selinux_handle();

	    char* con = NULL;

	    try
	    {
		string path(subvolume_dir.fullname() + "/" SNAPSHOTS_NAME);

		con = selabel_handle->selabel_lookup(path, mode);
		if (con)
		{
		    // race free mkdir with correct Selinux context preset
		    DefaultSelinuxFileContext defcon(con);
		    createLvmConfig(subvolume_dir, mode);
		}
		else
		{
		    y2deb("Selinux policy does not define context for path: " << path);

		    // race free mkdir with correct Selinux context preset even in case
		    // Selinux policy does not define context for the path
		    SnapperContexts scontexts;
		    DefaultSelinuxFileContext defcon(scontexts.subvolume_context());

		    createLvmConfig(subvolume_dir, mode);
		}

		freecon(con);

		return;
	    }
	    catch (const SelinuxException& e)
	    {
		SN_CAUGHT(e);
		freecon(con);
		// fall through intentional
	    }
	    catch (const CreateConfigFailedException& e)
	    {
		SN_CAUGHT(e);
		freecon(con);
		SN_RETHROW(e);
	    }
	}
#endif

	createLvmConfig(subvolume_dir, mode);
    }


    void
    Lvm::deleteConfig() const
    {
	SDir subvolume_dir = openSubvolumeDir();

	int r1 = subvolume_dir.rmdir(SNAPSHOTS_NAME);
	if (r1 != 0)
	{
	    y2err("rmdir '" SNAPSHOTS_NAME "' failed errno:" << errno << " (" << strerror(errno) << ")");
	    SN_THROW(DeleteConfigFailedException("rmdir failed"));
	}
    }


    string
    Lvm::snapshotDir(unsigned int num) const
    {
	return (subvolume == "/" ? "" : subvolume) + "/" SNAPSHOTS_NAME "/" + decString(num) +
	    "/" SNAPSHOT_NAME;
    }


    SDir
    Lvm::openInfosDir() const
    {
	SDir subvolume_dir = openSubvolumeDir();
	SDir infos_dir(subvolume_dir, SNAPSHOTS_NAME);

	struct stat stat;
	if (infos_dir.stat(&stat) != 0)
	{
	    SN_THROW(IOErrorException("stat on .snapshots failed"));
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
    Lvm::openSnapshotDir(unsigned int num) const
    {
	SDir info_dir = openInfoDir(num);
	SDir snapshot_dir(info_dir, SNAPSHOT_NAME);

	return snapshot_dir;
    }


    string
    Lvm::snapshotLvName(unsigned int num) const
    {
	return lv_name + "-snapshot" + decString(num);
    }


    void
    Lvm::createSnapshot(unsigned int num, unsigned int num_parent, bool read_only, bool quota,
			bool empty) const
    {
	if (num_parent != 0)
	    SN_THROW(UnsupportedException());

	SDir info_dir = openInfoDir(num);
	int r1 = info_dir.mkdir(SNAPSHOT_NAME, 0755);
	if (r1 != 0 && errno != EEXIST)
	{
	    y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    SN_THROW(CreateSnapshotFailedException());
	}

	try
	{
	    cache->create_snapshot(vg_name, lv_name, snapshotLvName(num), read_only);
	}
	catch (const LvmCacheException& e)
	{
	    SN_CAUGHT(e);
	    y2deb(cache);
	    SN_THROW(CreateSnapshotFailedException());
	}
    }


    void
    Lvm::deleteSnapshot(unsigned int num) const
    {
	try
	{
	    cache->delete_snapshot(vg_name, snapshotLvName(num));
	}
	catch (const LvmCacheException& e)
	{
	    SN_CAUGHT(e);
	    y2deb(cache);
	    SN_THROW(DeleteSnapshotFailedException());
	}

	SDir info_dir = openInfoDir(num);
	if (info_dir.rmdir(SNAPSHOT_NAME) < 0)
	     y2err("rmdir '" SNAPSHOT_NAME "' failed errno: " << errno << " (" << stringerror(errno) << ")");

	SDir infos_dir = openInfosDir();
	if (infos_dir.rmdir(decString(num)) < 0)
	     y2err("rmdir '" << num << "' failed errno: " << errno << " (" << stringerror(errno) << ")");
    }


    bool
    Lvm::isSnapshotMounted(unsigned int num) const
    {
	bool mounted = false;
	MtabData mtab_data;

	if (!getMtabData(snapshotDir(num), mounted, mtab_data))
	    SN_THROW(IsSnapshotMountedFailedException());

	return mounted;
    }


    void
    Lvm::mountSnapshot(unsigned int num) const
    {
	boost::unique_lock<boost::mutex> lock(mount_mutex);

	if (isSnapshotMounted(num))
	    return;

	try
	{
	    activateSnapshot(vg_name, snapshotLvName(num));
	}
	catch (const LvmActivationException& e)
	{
	    SN_CAUGHT(e);
	    SN_THROW(MountSnapshotFailedException());
	}

	SDir snapshot_dir = openSnapshotDir(num);

	if (!mount(getDevice(num), snapshot_dir, mount_type, mount_options))
	    SN_THROW(MountSnapshotFailedException());
    }


    void
    Lvm::umountSnapshot(unsigned int num) const
    {
	boost::unique_lock<boost::mutex> lock(mount_mutex);

	if (isSnapshotMounted(num))
	{
	    SDir info_dir = openInfoDir(num);

	    if (!umount(info_dir, SNAPSHOT_NAME))
		SN_THROW(UmountSnapshotFailedException());
	}

	try
	{
	    deactivateSnapshot(vg_name, snapshotLvName(num));
	}
	catch (const LvmDeactivatationException& e)
	{
	    SN_CAUGHT(e);
	    y2war("Couldn't deactivate: " << vg_name << "/" << lv_name);
	}
    }


    bool
    Lvm::isSnapshotReadOnly(unsigned int num) const
    {
	try
	{
	    return cache->is_read_only(vg_name, snapshotLvName(num));
	}
	catch (const LvmCacheException& e)
	{
	    SN_CAUGHT(e);
	    y2deb(cache);
	    SN_THROW(IOErrorException("query read-only failed"));
	    __builtin_unreachable();
	}
    }


    void
    Lvm::setSnapshotReadOnly(unsigned int num, bool read_only) const
    {
	try
	{
	    cache->set_read_only(vg_name, snapshotLvName(num), read_only);
	}
	catch (const LvmCacheException& e)
	{
	    SN_CAUGHT(e);
	    y2deb(cache);
	    SN_THROW(IOErrorException("set read-only failed"));
	}
    }


    bool
    Lvm::checkSnapshot(unsigned int num) const
    {
	return detectInactiveSnapshot(vg_name, snapshotLvName(num));
    }


    bool
    Lvm::detectThinVolumeNames(const MtabData& mtab_data)
    {
	try
	{
	    pair<string, string> names = LvmUtils::split_device_name(mtab_data.device);

	    vg_name = names.first;
	    lv_name = names.second;
	}
	catch (const runtime_error& e)
	{
	    y2err("could not detect lvm names from '" << mtab_data.device << "'");
	    return false;
	}

	try
	{
	    cache->add_or_update(vg_name, lv_name);
	}
	catch (const LvmCacheException& e)
	{
	    SN_CAUGHT(e);
	    y2deb(cache);
	    return false;
	}

	return cache->contains_thin(vg_name, lv_name);
    }


    string
    Lvm::getDevice(unsigned int num) const
    {
	return DEV_MAPPER_DIR "/" + boost::replace_all_copy(vg_name, "-", "--") + "-" +
	    boost::replace_all_copy(snapshotLvName(num), "-", "--");
    }


    void
    Lvm::activateSnapshot(const string& vg_name, const string& lv_name) const
    {
	try
	{
	    cache->activate(vg_name, lv_name);
	}
	catch (const LvmCacheException& e)
	{
	    SN_CAUGHT(e);
	    y2deb(cache);
	    SN_THROW(LvmActivationException());
	}
    }


    void
    Lvm::deactivateSnapshot(const string& vg_name, const string& lv_name) const
    {
	try
	{
	    cache->deactivate(vg_name, lv_name);
	}
	catch (const LvmCacheException& e)
	{
	    SN_CAUGHT(e);
	    y2deb(cache);
	    SN_THROW(LvmDeactivatationException());
	}
    }


    bool
    Lvm::detectInactiveSnapshot(const string& vg_name, const string& lv_name) const
    {
	return cache->contains(vg_name, lv_name);
    }


    LvmCapabilities::LvmCapabilities()
    {
	SystemCmd cmd({ LVM_BIN, "version" });

	if (cmd.retcode() != 0 || cmd.get_stdout().empty())
	{
	    y2war("Couldn't get LVM version info");
	}
	else
	{
	    static const regex rx(".*LVM[[:space:]]+version:[[:space:]]+([0-9]+)\\.([0-9]+)\\.([0-9]+).*$",
				  regex::extended);
	    smatch match;

	    if (!regex_search(cmd.get_stdout().front(), match, rx))
	    {
		y2war("LVM version format didn't match");
	    }
	    else
	    {
		uint16_t maj, min, rev;

		match[1] >> maj;
		match[2] >> min;
		match[3] >> rev;

		lvm_version version(maj, min, rev);

		if (version >= lvm_version(2, 2, 99))
		    ignoreactivationskip = "--ignoreactivationskip";
	    }
	}
    }


    bool
    operator>=(const lvm_version& a, const lvm_version& b)
    {
	return a.version >= b.version;
    }


    LvmCapabilities*
    LvmCapabilities::get_lvm_capabilities()
    {
	/*
	 * NOTE: verify only one thread can access
	 * 	 this section at the same time!
	 */
	static LvmCapabilities caps;

	return &caps;
    }

}
