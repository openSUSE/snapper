/*
 * Copyright (c) [2016-2025] SUSE LLC
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


#include "proxy-dbus.h"
#include "commands.h"
#include "../utils/text.h"


namespace snapper
{

using namespace std;


ProxySnapshotDbus::ProxySnapshotDbus(ProxySnapshotsDbus* backref, unsigned int num)
    : backref(backref), num(num)
{
    XSnapshot x = command_get_xsnapshot(conn(), configName(), num);

    type = x.type;
    date = x.date;
    uid = x.uid;
    pre_num = x.pre_num;
    description = x.description;
    cleanup = x.cleanup;
    userdata = x.userdata;
}


ProxySnapshotDbus::ProxySnapshotDbus(ProxySnapshotsDbus* backref, SnapshotType type, unsigned int num,
				     time_t date, uid_t uid, unsigned int pre_num,
				     const string& description, const string& cleanup,
				     const map<string, string>& userdata)
    : backref(backref), type(type), num(num), date(date), uid(uid), pre_num(pre_num),
      description(description), cleanup(cleanup), userdata(userdata)
{
}


bool
ProxySnapshotDbus::isReadOnly() const
{
    return command_is_snapshot_read_only(conn(), configName(), num);
}


void
ProxySnapshotDbus::setReadOnly(bool read_only, Plugins::Report& report)
{
    command_set_snapshot_read_only(conn(), configName(), num, read_only);
}


void
ProxySnapperDbus::calculateUsedSpace() const
{
    command_calculate_used_space(conn(), config_name);
}


uint64_t
ProxySnapshotDbus::getUsedSpace() const
{
    return command_get_used_space(conn(), configName(), num);
}


void
ProxySnapperDbus::lock_config() const
{
    command_lock_config(conn(), config_name);
}


void
ProxySnapperDbus::unlock_config() const
{
    command_unlock_config(conn(), config_name);
}


string
ProxySnapshotDbus::mountFilesystemSnapshot(bool user_request) const
{
    return command_mount_snapshot(conn(), configName(), num, user_request);
}


void
ProxySnapshotDbus::umountFilesystemSnapshot(bool user_request) const
{
    command_umount_snapshot(conn(), configName(), num, user_request);
}


DBus::Connection&
ProxySnapshotDbus::conn() const
{
    return backref->conn();
}


const string&
ProxySnapshotDbus::configName() const
{
    return backref->configName();
}


ProxySnapshotsDbus::ProxySnapshotsDbus(ProxySnapperDbus* backref)
    : backref(backref)
{
    XSnapshots tmp = command_list_xsnapshots(conn(), configName());
    for (XSnapshots::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
	proxy_snapshots.push_back(new ProxySnapshotDbus(this, it->getType(), it->getNum(), it->getDate(),
							it->getUid(), it->getPreNum(), it->getDescription(),
							it->getCleanup(), it->getUserdata()));
}


ProxySnapshots::const_iterator
ProxySnapshotsDbus::getDefault() const
{
    pair<bool, unsigned int> tmp = command_get_default_snapshot(conn(), configName());

    return tmp.first ? find(tmp.second) : end();
}


ProxySnapshots::iterator
ProxySnapshotsDbus::getDefault()
{
    pair<bool, unsigned int> tmp = command_get_default_snapshot(conn(), configName());

    return tmp.first ? find(tmp.second) : end();
}


ProxySnapshots::iterator
ProxySnapshotsDbus::getActive()
{
    pair<bool, unsigned int> tmp = command_get_active_snapshot(conn(), configName());

    return tmp.first ? find(tmp.second) : end();
}


ProxySnapshots::const_iterator
ProxySnapshotsDbus::getActive() const
{
    pair<bool, unsigned int> tmp = command_get_active_snapshot(conn(), configName());

    return tmp.first ? find(tmp.second) : end();
}


DBus::Connection&
ProxySnapshotsDbus::conn() const
{
    return backref->conn();
}


const string&
ProxySnapshotsDbus::configName() const
{
    return backref->config_name;
}


ProxyConfig
ProxySnapperDbus::getConfig() const
{
    XConfigInfo tmp = command_get_xconfig(conn(), config_name);

    return ProxyConfig(tmp.raw);
}


void
ProxySnapperDbus::setConfig(const ProxyConfig& proxy_config)
{
    command_set_xconfig(conn(), config_name, proxy_config.getAllValues());
}


void
ProxySnapperDbus::setupQuota()
{
    command_setup_quota(conn(), config_name);
}


void
ProxySnapperDbus::prepareQuota() const
{
    command_prepare_quota(conn(), config_name);
}


QuotaData
ProxySnapperDbus::queryQuotaData() const
{
    return command_query_quota(conn(), config_name);
}


FreeSpaceData
ProxySnapperDbus::queryFreeSpaceData() const
{
    return command_query_free_space(conn(), config_name);
}


ProxySnapshots::const_iterator
ProxySnapperDbus::createSingleSnapshot(const SCD& scd, Plugins::Report& report)
{
    unsigned int num = command_create_single_snapshot(conn(), config_name, scd.description,
						      scd.cleanup, scd.userdata);

    proxy_snapshots.emplace_back(new ProxySnapshotDbus(&proxy_snapshots, num));

    return prev(proxy_snapshots.end());
}


ProxySnapshots::const_iterator
ProxySnapperDbus::createSingleSnapshot(ProxySnapshots::const_iterator parent, const SCD& scd, Plugins::Report& report)
{
    unsigned int num = command_create_single_snapshot_v2(conn(), config_name, parent->getNum(),
							 scd.read_only, scd.description, scd.cleanup,
							 scd.userdata);

    proxy_snapshots.emplace_back(new ProxySnapshotDbus(&proxy_snapshots, num));

    return prev(proxy_snapshots.end());
}


ProxySnapshots::const_iterator
ProxySnapperDbus::createSingleSnapshotOfDefault(const SCD& scd, Plugins::Report& report)
{
    unsigned int num = command_create_single_snapshot_of_default(conn(), config_name, scd.read_only,
								 scd.description, scd.cleanup,
								 scd.userdata);

    proxy_snapshots.emplace_back(new ProxySnapshotDbus(&proxy_snapshots, num));

    return prev(proxy_snapshots.end());
}


ProxySnapshots::const_iterator
ProxySnapperDbus::createPreSnapshot(const SCD& scd, Plugins::Report& report)
{
    unsigned int num = command_create_pre_snapshot(conn(), config_name, scd.description,
						   scd.cleanup, scd.userdata);

    proxy_snapshots.emplace_back(new ProxySnapshotDbus(&proxy_snapshots, num));

    return prev(proxy_snapshots.end());
}


ProxySnapshots::const_iterator
ProxySnapperDbus::createPostSnapshot(ProxySnapshots::const_iterator pre, const SCD& scd, Plugins::Report& report)
{
    unsigned int num = command_create_post_snapshot(conn(), config_name, pre->getNum(),
						    scd.description, scd.cleanup, scd.userdata);

    proxy_snapshots.emplace_back(new ProxySnapshotDbus(&proxy_snapshots, num));

    return prev(proxy_snapshots.end());
}


void
ProxySnapperDbus::modifySnapshot(ProxySnapshots::iterator snapshot, const SMD& smd, Plugins::Report& report)
{
    command_set_snapshot(conn(), config_name, snapshot->getNum(), smd);
}


void
ProxySnapperDbus::deleteSnapshots(vector<ProxySnapshots::iterator> snapshots, bool verbose, Plugins::Report& report)
{
    vector<unsigned int> nums;
    for (const ProxySnapshots::iterator& proxy_snapshot : snapshots)
	nums.push_back(proxy_snapshot->getNum());

    command_delete_snapshots(conn(), config_name, nums, verbose);

    ProxySnapshots& proxy_snapshots = getSnapshots();
    for (ProxySnapshots::iterator& proxy_snapshot : snapshots)
	proxy_snapshots.erase(proxy_snapshot);
}


ProxyComparison
ProxySnapperDbus::createComparison(const ProxySnapshot& lhs, const ProxySnapshot& rhs, bool mount)
{
    return ProxyComparison(new ProxyComparisonDbus(this, lhs, rhs, mount));
}


void
ProxySnapperDbus::syncFilesystem() const
{
    command_sync(conn(), config_name);
}


DBus::Connection&
ProxySnapperDbus::conn() const
{
    return backref->conn;
}


void
ProxySnappersDbus::createConfig(const string& config_name, const string& subvolume,
				const string& fstype, const string& template_name, Plugins::Report& report)
{
    command_create_config(conn, config_name, subvolume, fstype, template_name);
}


void
ProxySnappersDbus::deleteConfig(const string& config_name, Plugins::Report& report)
{
    command_delete_config(conn, config_name);
}


ProxySnapper*
ProxySnappersDbus::getSnapper(const string& config_name)
{
    for (unique_ptr<ProxySnapperDbus>& proxy_snapper : proxy_snappers)
    {
	if (proxy_snapper->config_name == config_name)
	    return proxy_snapper.get();
    }

    ProxySnapperDbus* ret = new ProxySnapperDbus(this, config_name);
    proxy_snappers.emplace_back(ret);
    return ret;
}


map<string, ProxyConfig>
ProxySnappersDbus::getConfigs() const
{
    map<string, ProxyConfig> ret;

    vector<XConfigInfo> config_infos = command_list_xconfigs(conn);
    for (XConfigInfo& x : config_infos)
	ret.emplace(make_pair(x.config_name, x.raw));

    return ret;
}


Plugins::Report
ProxySnappersDbus::get_plugins_report() const
{
    Plugins::Report ret;

    try
    {
	for (const XReport& xreport : command_get_plugins_report(conn))
	    ret.entries.emplace_back(xreport.name, xreport.args, xreport.exit_status);
    }
    catch (const DBus::ErrorException& e)
    {
	SN_CAUGHT(e);

	// If snapper was just updated and the old snapperd is still
	// running it might not know the GetPluginsReport method.

	if (strcmp(e.name(), "error.unknown_method") != 0)
	    SN_RETHROW(e);
    }

    return ret;
}


vector<string>
ProxySnappersDbus::debug() const
{
    return command_debug(conn);
}


ProxyComparisonDbus::ProxyComparisonDbus(ProxySnapperDbus* backref, const ProxySnapshot& lhs,
					 const ProxySnapshot& rhs, bool mount)
    : backref(backref), lhs(lhs), rhs(rhs), files(&file_paths)
{
    command_create_comparison(conn(), configName(), lhs.getNum(), rhs.getNum());

    file_paths.system_path = command_get_mount_point(backref->conn(), backref->config_name, 0);

    if (mount)
    {
	if (!lhs.isCurrent())
	    file_paths.pre_path = command_mount_snapshot(backref->conn(), backref->config_name,
							 lhs.getNum(), false);
	else
	    file_paths.pre_path = file_paths.system_path;

	if (!rhs.isCurrent())
	    file_paths.post_path = command_mount_snapshot(backref->conn(), backref->config_name,
							  rhs.getNum(), false);
	else
	    file_paths.post_path = file_paths.system_path;
    }

    vector<XFile> tmp1;

    try
    {
	tmp1 = command_get_xfiles_by_pipe(backref->conn(), backref->config_name, lhs.getNum(),
					  rhs.getNum());
    }
    catch (const DBus::ErrorException& e)
    {
	SN_CAUGHT(e);

	// If snapper was just updated and the old snapperd is still running it might not
	// know the GetFilesByPipe method.

	if (strcmp(e.name(), "error.unknown_method") != 0)
	    SN_RETHROW(e);

	tmp1 = command_get_xfiles(backref->conn(), backref->config_name, lhs.getNum(),
				  rhs.getNum());
    }

    vector<File> tmp2;
    tmp2.reserve(tmp1.size());

    for (const XFile& xfile : tmp1)
	tmp2.emplace_back(&file_paths, xfile.name, xfile.status);

    files = Files(&file_paths, tmp2);
}


ProxyComparisonDbus::~ProxyComparisonDbus()
{
    command_delete_comparison(conn(), configName(), lhs.getNum(), rhs.getNum());
}


DBus::Connection&
ProxyComparisonDbus::conn() const
{
    return backref->conn();
}


const string&
ProxyComparisonDbus::configName() const
{
    return backref->config_name;
}


ProxySnappers
ProxySnappers::createDbus()
{
    return ProxySnappers(new ProxySnappersDbus());
}

}
