/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) 2016 SUSE LLC
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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/statvfs.h>
#include <glob.h>
#include <string.h>
#include <mntent.h>
#include <sys/acl.h>
#include <acl/libacl.h>
#include <set>
#include <boost/algorithm/string.hpp>

#include "snapper/Snapper.h"
#include "snapper/Comparison.h"
#include "snapper/AppUtil.h"
#include "snapper/Enum.h"
#include "snapper/Filesystem.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/File.h"
#include "snapper/AsciiFile.h"
#include "snapper/Exception.h"
#include "snapper/Hooks.h"
#include "snapper/Btrfs.h"
#include "snapper/BtrfsUtils.h"
#ifdef ENABLE_SELINUX
#include "snapper/Selinux.h"
#include "snapper/Regex.h"
#endif


namespace snapper
{
    using namespace std;


    ConfigInfo::ConfigInfo(const string& config_name, const string& root_prefix)
	: SysconfigFile(prepend_root_prefix(root_prefix, CONFIGSDIR "/" + config_name)),
	  config_name(config_name), subvolume("/")
    {
	if (!getValue(KEY_SUBVOLUME, subvolume))
	    SN_THROW(InvalidConfigException());
    }


    void
    ConfigInfo::checkKey(const string& key) const
    {
	if (key == KEY_SUBVOLUME || key == KEY_FSTYPE)
	    SN_THROW(InvalidConfigdataException());

	try
	{
	    SysconfigFile::checkKey(key);
	}
	catch (const InvalidKeyException& e)
	{
	    SN_THROW(InvalidConfigdataException());
	}
    }


    Snapper::Snapper(const string& config_name, const string& root_prefix, bool disable_filters)
	: config_info(NULL), filesystem(NULL), snapshots(this), selabel_handle(NULL)
    {
	y2mil("Snapper constructor");
	y2mil("libsnapper version " VERSION);
	y2mil("config_name:" << config_name << " disable_filters:" << disable_filters);

#ifdef ENABLE_SELINUX
	try
	{
	    selabel_handle = SelinuxLabelHandle::get_selinux_handle();
	}
	catch (const SelinuxException& e)
	{
	    SN_RETHROW(e);
	}
#endif

	try
	{
	    config_info = new ConfigInfo(config_name, root_prefix);
	}
	catch (const FileNotFoundException& e)
	{
	    SN_THROW(ConfigNotFoundException());
	}

	filesystem = Filesystem::create(*config_info, root_prefix);

	syncSelinuxContexts();

	bool sync_acl;
	if (config_info->getValue(KEY_SYNC_ACL, sync_acl) && sync_acl == true)
	    syncAcl();

	y2mil("subvolume:" << config_info->getSubvolume() << " filesystem:" <<
	      filesystem->fstype());

	if (!disable_filters)
	    loadIgnorePatterns();

	snapshots.initialize();
    }


    Snapper::~Snapper()
    {
	y2mil("Snapper destructor");

	for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	    try
	    {
		it->handleUmountFilesystemSnapshot();
	    }
	    catch (const UmountSnapshotFailedException& e)
	    {
	    }
	}

	delete filesystem;
	delete config_info;
    }


    void
    Snapper::loadIgnorePatterns()
    {
	const list<string> files = glob(FILTERSDIR "/*.txt", GLOB_NOSORT);
	for (list<string>::const_iterator it = files.begin(); it != files.end(); ++it)
	{
	    try
	    {
		AsciiFileReader asciifile(*it);

		string line;
		while (asciifile.getline(line))
		    if (!line.empty())
			ignore_patterns.push_back(line);
	    }
	    catch (const FileNotFoundException& e)
	    {
	    }
	}

	y2mil("number of ignore patterns:" << ignore_patterns.size());
    }


    // Directory of which snapshots are made, e.g. "/" or "/home".
    string
    Snapper::subvolumeDir() const
    {
	return config_info->getSubvolume();
    }


    SDir
    Snapper::openSubvolumeDir() const
    {
	return filesystem->openSubvolumeDir();
    }


    SDir
    Snapper::openInfosDir() const
    {
	return filesystem->openInfosDir();
    }


    Snapshots::const_iterator
    Snapper::getSnapshotCurrent() const
    {
	return snapshots.getSnapshotCurrent();
    }


    Snapshots::iterator
    Snapper::createSingleSnapshot(const SCD& scd)
    {
	return snapshots.createSingleSnapshot(scd);
    }


    Snapshots::iterator
    Snapper::createSingleSnapshot(Snapshots::const_iterator parent, const SCD& scd)
    {
	if (parent == snapshots.end())
	    SN_THROW(IllegalSnapshotException());

	return snapshots.createSingleSnapshot(parent, scd);
    }


    Snapshots::iterator
    Snapper::createSingleSnapshotOfDefault(const SCD& scd)
    {
	return snapshots.createSingleSnapshotOfDefault(scd);
    }


    Snapshots::iterator
    Snapper::createPreSnapshot(const SCD& scd)
    {
	return snapshots.createPreSnapshot(scd);
    }


    Snapshots::iterator
    Snapper::createPostSnapshot(Snapshots::const_iterator pre, const SCD& scd)
    {
	return snapshots.createPostSnapshot(pre, scd);
    }


    void
    Snapper::modifySnapshot(Snapshots::iterator snapshot, const SMD& smd)
    {
	snapshots.modifySnapshot(snapshot, smd);
    }


    void
    Snapper::deleteSnapshot(Snapshots::iterator snapshot)
    {
	snapshots.deleteSnapshot(snapshot);
    }


    ConfigInfo
    Snapper::getConfig(const string& config_name, const string& root_prefix)
    {
	return ConfigInfo(config_name, root_prefix);
    }


    list<ConfigInfo>
    Snapper::getConfigs(const string& root_prefix)
    {
	y2mil("Snapper get-configs");
	y2mil("libsnapper version " VERSION);

	list<ConfigInfo> config_infos;

	try
	{
	    SysconfigFile sysconfig(prepend_root_prefix(root_prefix, SYSCONFIGFILE));
	    vector<string> config_names;
	    sysconfig.getValue("SNAPPER_CONFIGS", config_names);

	    for (vector<string>::const_iterator it = config_names.begin(); it != config_names.end(); ++it)
	    {
		try
		{
		    config_infos.push_back(getConfig(*it, root_prefix));
		}
		catch (const FileNotFoundException& e)
		{
		    y2err("config '" << *it << "' not found");
		}
		catch (const InvalidConfigException& e)
		{
		    y2err("config '" << *it << "' is invalid");
		}
	    }
	}
	catch (const FileNotFoundException& e)
	{
	    SN_THROW(ListConfigsFailedException("sysconfig-file not found"));
	}

	return config_infos;
    }


    void
    Snapper::createConfig(const string& config_name, const string& root_prefix,
			  const string& subvolume, const string& fstype,
			  const string& template_name)
    {
	y2mil("Snapper create-config");
	y2mil("libsnapper version " VERSION);
	y2mil("config_name:" << config_name << " subvolume:" << subvolume <<
	      " fstype:" << fstype << " template_name:" << template_name);

	if (config_name.empty() || config_name.find_first_of(", \t") != string::npos)
	{
	    SN_THROW(CreateConfigFailedException("illegal config name"));
	}

	if (!boost::starts_with(subvolume, "/") || !checkDir(subvolume))
	{
	    SN_THROW(CreateConfigFailedException("illegal subvolume"));
	}

	list<ConfigInfo> configs = getConfigs(root_prefix);
	for (list<ConfigInfo>::const_iterator it = configs.begin(); it != configs.end(); ++it)
	{
	    if (it->getSubvolume() == subvolume)
	    {
		SN_THROW(CreateConfigFailedException("subvolume already covered"));
	    }
	}

	if (access(string(CONFIGTEMPLATEDIR "/" + template_name).c_str(), R_OK) != 0)
	{
	    SN_THROW(CreateConfigFailedException("cannot access template config"));
	}

	unique_ptr<Filesystem> filesystem;
	try
	{
	    filesystem.reset(Filesystem::create(fstype, subvolume, ""));
	}
	catch (const InvalidConfigException& e)
	{
	    SN_THROW(CreateConfigFailedException("invalid filesystem type"));
	}
	catch (const ProgramNotInstalledException& e)
	{
	    SN_THROW(CreateConfigFailedException(e.what()));
	}

	try
	{
	    SysconfigFile sysconfig(SYSCONFIGFILE);
	    vector<string> config_names;
	    sysconfig.getValue("SNAPPER_CONFIGS", config_names);
	    if (find(config_names.begin(), config_names.end(), config_name) != config_names.end())
	    {
		SN_THROW(CreateConfigFailedException("config already exists"));
	    }

	    config_names.push_back(config_name);
	    sysconfig.setValue("SNAPPER_CONFIGS", config_names);
	}
	catch (const FileNotFoundException& e)
	{
	    SN_THROW(CreateConfigFailedException("sysconfig-file not found"));
	}

	try
	{
	    SysconfigFile config(CONFIGTEMPLATEDIR "/" + template_name);

	    config.setName(CONFIGSDIR "/" + config_name);

	    config.setValue(KEY_SUBVOLUME, subvolume);
	    config.setValue(KEY_FSTYPE, filesystem->fstype());
	}
	catch (const FileNotFoundException& e)
	{
	    SN_THROW(CreateConfigFailedException("modifying config failed"));
	}

	try
	{
	    filesystem->createConfig();
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    SysconfigFile sysconfig(SYSCONFIGFILE);
	    vector<string> config_names;
	    sysconfig.getValue("SNAPPER_CONFIGS", config_names);
	    config_names.erase(remove(config_names.begin(), config_names.end(), config_name),
			       config_names.end());
	    sysconfig.setValue("SNAPPER_CONFIGS", config_names);

	    SystemCmd cmd(RMBIN " " + quote(CONFIGSDIR "/" + config_name));

	    SN_RETHROW(e);
	}

	Hooks::create_config(subvolume, filesystem.get());
    }


    void
    Snapper::deleteConfig(const string& config_name, const string& root_prefix)
    {
	y2mil("Snapper delete-config");
	y2mil("libsnapper version " VERSION);

	unique_ptr<Snapper> snapper(new Snapper(config_name, root_prefix));

	Hooks::delete_config(snapper->subvolumeDir(), snapper->getFilesystem());

	Snapshots& snapshots = snapper->getSnapshots();
	for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); )
	{
	    Snapshots::iterator tmp = it++;

	    if (tmp->isCurrent())
		continue;

	    try
	    {
		snapper->deleteSnapshot(tmp);
	    }
	    catch (const DeleteSnapshotFailedException& e)
	    {
		// ignore, Filesystem->deleteConfig will fail anyway
	    }
	}

	try
	{
	    snapper->getFilesystem()->deleteConfig();
	}
	catch (const DeleteConfigFailedException& e)
	{
	    SN_THROW(DeleteConfigFailedException("deleting snapshot failed"));
	}

	SystemCmd cmd1(RMBIN " " + quote(CONFIGSDIR "/" + config_name));
	if (cmd1.retcode() != 0)
	{
	    SN_THROW(DeleteConfigFailedException("deleting config-file failed"));
	}

	try
	{
	    SysconfigFile sysconfig(SYSCONFIGFILE);
	    vector<string> config_names;
	    sysconfig.getValue("SNAPPER_CONFIGS", config_names);
	    config_names.erase(remove(config_names.begin(), config_names.end(), config_name),
			       config_names.end());
	    sysconfig.setValue("SNAPPER_CONFIGS", config_names);
	}
	catch (const FileNotFoundException& e)
	{
	    SN_THROW(DeleteConfigFailedException("sysconfig-file not found"));
	}
    }


    void
    Snapper::setConfigInfo(const map<string, string>& raw)
    {
	for (map<string, string>::const_iterator it = raw.begin(); it != raw.end(); ++it)
	    config_info->setValue(it->first, it->second);

	config_info->save();

	filesystem->evalConfigInfo(*config_info);

	if (raw.find(KEY_ALLOW_USERS) != raw.end() || raw.find(KEY_ALLOW_GROUPS) != raw.end() ||
	    raw.find(KEY_SYNC_ACL) != raw.end())
	{
	    bool sync_acl;
	    if (config_info->getValue(KEY_SYNC_ACL, sync_acl) && sync_acl == true)
		syncAcl();
	}
    }


    void
    Snapper::syncAcl() const
    {
	vector<uid_t> uids;
	vector<string> users;
	if (config_info->getValue(KEY_ALLOW_USERS, users))
	{
	    for (vector<string>::const_iterator it = users.begin(); it != users.end(); ++it)
	    {
		uid_t uid;
		if (!get_user_uid(it->c_str(), uid))
		    SN_THROW(InvalidUserException());
		uids.push_back(uid);
	    }
	}

	vector<gid_t> gids;
	vector<string> groups;
	if (config_info->getValue(KEY_ALLOW_GROUPS, groups))
	{
	    for (vector<string>::const_iterator it = groups.begin(); it != groups.end(); ++it)
	    {
		gid_t gid;
		if (!get_group_gid(it->c_str(), gid))
		    SN_THROW(InvalidGroupException());
		gids.push_back(gid);
	    }
	}

	syncAcl(uids, gids);
    }


    void
    Snapper::syncFilesystem() const
    {
	filesystem->sync();
    }


    static void
    set_acl_permissions(acl_entry_t entry)
    {
	acl_permset_t permset;
	if (acl_get_permset(entry, &permset) != 0)
	    SN_THROW(AclException());

	if (acl_add_perm(permset, ACL_READ) != 0 || acl_delete_perm(permset, ACL_WRITE) != 0 ||
	    acl_add_perm(permset, ACL_EXECUTE) != 0)
	    SN_THROW(AclException());
    };


    static void
    add_acl_permissions(acl_t* acl, acl_tag_t tag, const void* qualifier)
    {
	acl_entry_t entry;
	if (acl_create_entry(acl, &entry) != 0)
	    SN_THROW(AclException());

	if (acl_set_tag_type(entry, tag) != 0)
	    SN_THROW(AclException());

	if (acl_set_qualifier(entry, qualifier) != 0)
	    SN_THROW(AclException());

	set_acl_permissions(entry);
    };


    void
    Snapper::syncAcl(const vector<uid_t>& uids, const vector<gid_t>& gids) const
    {
	SDir infos_dir = openInfosDir();

	acl_t orig_acl = acl_get_fd(infos_dir.fd());
	if (!orig_acl)
	    SN_THROW(AclException());

	acl_t acl = acl_dup(orig_acl);
	if (!acl)
	    SN_THROW(AclException());

	set<uid_t> remaining_uids = set<uid_t>(uids.begin(), uids.end());
	set<gid_t> remaining_gids = set<gid_t>(gids.begin(), gids.end());

	acl_entry_t entry;
	if (acl_get_entry(acl, ACL_FIRST_ENTRY, &entry) == 1)
	{
	    do {

		acl_tag_t tag;
		if (acl_get_tag_type(entry, &tag) != 0)
		    SN_THROW(AclException());

		switch (tag)
		{
		    case ACL_USER: {

			uid_t* uid = (uid_t*) acl_get_qualifier(entry);
			if (!uid)
			    SN_THROW(AclException());

			if (contains(remaining_uids, *uid))
			{
			    remaining_uids.erase(*uid);

			    set_acl_permissions(entry);
			}
			else
			{
			    if (acl_delete_entry(acl, entry) != 0)
				SN_THROW(AclException());
			}

		    } break;

		    case ACL_GROUP: {

			gid_t* gid = (gid_t*) acl_get_qualifier(entry);
			if (!gid)
			    SN_THROW(AclException());

			if (contains(remaining_gids, *gid))
			{
			    remaining_gids.erase(*gid);

			    set_acl_permissions(entry);
			}
			else
			{
			    if (acl_delete_entry(acl, entry) != 0)
				SN_THROW(AclException());
			}

		    } break;

		}

	    } while (acl_get_entry(acl, ACL_NEXT_ENTRY, &entry) == 1);
	}

	for (set<uid_t>::const_iterator it = remaining_uids.begin(); it != remaining_uids.end(); ++it)
	{
	    add_acl_permissions(&acl, ACL_USER, &(*it));
	}

	for (set<gid_t>::const_iterator it = remaining_gids.begin(); it != remaining_gids.end(); ++it)
	{
	    add_acl_permissions(&acl, ACL_GROUP, &(*it));
	}

	if (acl_calc_mask(&acl) != 0)
	    SN_THROW(AclException());

	if (acl_cmp(orig_acl, acl) == 1)
	    if (acl_set_fd(infos_dir.fd(), acl) != 0)
		SN_THROW(AclException());

	if (acl_free(acl) != 0)
	    SN_THROW(AclException());
    }


    void
    Snapper::setupQuota()
    {
#ifdef ENABLE_BTRFS_QUOTA

	const Btrfs* btrfs = dynamic_cast<const Btrfs*>(getFilesystem());
	if (!btrfs)
	    SN_THROW(QuotaException("quota only supported with btrfs"));

	if (btrfs->getQGroup() != no_qgroup)
	    SN_THROW(QuotaException("qgroup already set"));

	SDir subvolume_dir = openSubvolumeDir();

	quota_enable(subvolume_dir.fd());

	qgroup_t qgroup = qgroup_find_free(subvolume_dir.fd(), 1);

	y2mil("free qgroup:" << format_qgroup(qgroup));

	qgroup_create(subvolume_dir.fd(), qgroup);

	setConfigInfo({ { "QGROUP", format_qgroup(qgroup) } });

#else

	SN_THROW(QuotaException("not implemented"));
	__builtin_unreachable();

#endif
    }


    void
    Snapper::prepareQuota() const
    {
#ifdef ENABLE_BTRFS_QUOTA

	const Btrfs* btrfs = dynamic_cast<const Btrfs*>(getFilesystem());
	if (!btrfs)
	    SN_THROW(QuotaException("quota only supported with btrfs"));

	if (btrfs->getQGroup() == no_qgroup)
	    SN_THROW(QuotaException("qgroup not set"));

	SDir subvolume_dir = openSubvolumeDir();

	vector<qgroup_t> children = qgroup_query_children(subvolume_dir.fd(), btrfs->getQGroup());
	sort(children.begin(), children.end());

	// Iterate all snapshot and ensure that those and only those with a
	// cleanup algorithm are included in the high level qgroup.

	for (const Snapshot& snapshot : snapshots)
	{
	    if (snapshot.isCurrent())
		continue;

	    subvolid_t subvolid = get_id(snapshot.openSnapshotDir().fd());
	    qgroup_t qgroup = calc_qgroup(0, subvolid);

	    bool included = binary_search(children.begin(), children.end(), qgroup);

	    if (!snapshot.getCleanup().empty() && !included)
	    {
		qgroup_assign(subvolume_dir.fd(), qgroup, btrfs->getQGroup());
	    }
	    else if (snapshot.getCleanup().empty() && included)
	    {
		qgroup_remove(subvolume_dir.fd(), qgroup, btrfs->getQGroup());
	    }
	}

	quota_rescan(subvolume_dir.fd());

#else

	SN_THROW(QuotaException("not implemented"));
	__builtin_unreachable();

#endif
    }


    QuotaData
    Snapper::queryQuotaData() const
    {
#ifdef ENABLE_BTRFS_QUOTA

	const Btrfs* btrfs = dynamic_cast<const Btrfs*>(getFilesystem());
	if (!btrfs)
	    SN_THROW(QuotaException("quota only supported with btrfs"));

	if (btrfs->getQGroup() == no_qgroup)
	    SN_THROW(QuotaException("qgroup not set"));

	SDir subvolume_dir = openSubvolumeDir();

	// Tests have shown that without a rescan and sync here the quota data
	// is incorrect.

	quota_rescan(subvolume_dir.fd());
	sync(subvolume_dir.fd());

	struct statvfs64 fsbuf;
	if (fstatvfs64(subvolume_dir.fd(), &fsbuf) != 0)
	    SN_THROW(QuotaException("statvfs64 failed"));

	QuotaData quota_data;

	quota_data.size = fsbuf.f_blocks * fsbuf.f_bsize;

	QGroupUsage qgroup_usage = qgroup_query_usage(subvolume_dir.fd(), btrfs->getQGroup());
	quota_data.used = qgroup_usage.exclusive;

	y2mil("size:" << quota_data.size << " used:" << quota_data.used);

	if (quota_data.used > quota_data.size)
	    SN_THROW(QuotaException("impossible quota values"));

	return quota_data;

#else

	SN_THROW(QuotaException("not implemented"));
	__builtin_unreachable();

#endif
    }


    void
    Snapper::syncSelinuxContexts() const
    {
#ifdef ENABLE_SELINUX
	try
	{
	    SDir subvol_dir = openSubvolumeDir();
	    SDir infos_dir(subvol_dir, ".snapshots");

	    if (infos_dir.restorecon(selabel_handle))
	    {
		syncSelinuxContextsInInfosDir();
	    }
	    else
	    {
		SnapperContexts scons;

		if (infos_dir.fsetfilecon(scons.subvolume_context()))
		    syncSelinuxContextsInInfosDir();
	    }
	}
	catch (const SelinuxException& e)
	{
	    SN_CAUGHT(e);
	    // fall through intentional
	}
#endif
    }


    void
    Snapper::syncSelinuxContextsInInfosDir() const
    {
#ifdef ENABLE_SELINUX
	Regex rx("^[0-9]+$");
	Regex rx_filelist("^filelist-[0-9]+.txt$");

	y2deb("Syncing Selinux contexts in infos dir");

	SDir infos_dir = openInfosDir();

	vector<string> infos = infos_dir.entries();
	for (vector<string>::const_iterator it1 = infos.begin(); it1 != infos.end(); ++it1)
	{
	    if (!rx.match(*it1))
		continue;

	    SDir info_dir(infos_dir, *it1);
	    info_dir.restorecon(selabel_handle);

	    SFile info(info_dir, "info.xml");
	    info.restorecon(selabel_handle);

	    SFile snapshot_dir(info_dir, "snapshot");
	    snapshot_dir.restorecon(selabel_handle); // this usually fails w/ btrfs backend (it's RO)

	    vector<string> info_content = info_dir.entries();
	    for (vector<string>::const_iterator it2 = info_content.begin(); it2 != info_content.end(); ++it2)
	    {
		if (!rx_filelist.match(*it2))
		    continue;

		SFile fl(info_dir, *it2);
		fl.restorecon(selabel_handle);
	    }
	}
#endif
    }


    static bool
    is_subpath(const string& a, const string& b)
    {
	if (b == "/")
	    return true;

	size_t len = b.length();

	if (len > a.length())
	    return false;

	return (len == a.length() || a[len] == '/') && a.compare(0, len, b) == 0;
    }


    bool
    Snapper::detectFstype(const string& subvolume, string& fstype)
    {
	y2mil("subvolume:" << subvolume);

	if (!boost::starts_with(subvolume, "/") || !checkDir(subvolume))
	    return false;

	FILE* f = setmntent("/etc/mtab", "r");
	if (!f)
	{
	    y2err("setmntent failed");
	    return false;
	}

	fstype.clear();

	string best_match;

	struct mntent* m;
	while ((m = getmntent(f)))
	{
	    if (strcmp(m->mnt_type, "rootfs") == 0)
		continue;

	    if (strlen(m->mnt_dir) >= best_match.length() && is_subpath(subvolume, m->mnt_dir))
	    {
		best_match = m->mnt_dir;
		fstype = m->mnt_type;
	    }
	}

	endmntent(f);

	if (fstype == "ext4dev")
	    fstype = "ext4";

	y2mil("fstype:" << fstype);

	return !best_match.empty();
    }


    const char*
    Snapper::compileVersion()
    {
	return VERSION;
    }


    const char*
    Snapper::compileFlags()
    {
	return

#ifndef ENABLE_BTRFS
	    "no-"
#endif
	    "btrfs,"

#ifndef ENABLE_LVM
	    "no-"
#endif
	    "lvm,"

#ifndef ENABLE_EXT4
	    "no-"
#endif
	    "ext4,"

#ifndef ENABLE_XATTRS
	    "no-"
#endif
	    "xattrs,"

#ifndef ENABLE_ROLLBACK
	    "no-"
#endif
	    "rollback,"

#ifndef ENABLE_BTRFS_QUOTA
	    "no-"
#endif
	    "btrfs-quota,"

#ifndef ENABLE_SELINUX
	    "no-"
#endif
	    "selinux"

	    ;
    }

}
