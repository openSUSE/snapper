/*
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


#include <sstream>

#include "proxy-dbus.h"
#include "commands.h"
#include "utils/text.h"


#define SERVICE "org.opensuse.Snapper"
#define OBJECT "/org/opensuse/Snapper"
#define INTERFACE "org.opensuse.Snapper"


using namespace std;


ProxySnapshotDbus::ProxySnapshotDbus(ProxySnapshotsDbus* backref, unsigned int num)
    : backref(backref), num(num)
{
    XSnapshot x = command_get_xsnapshot(conn(), config_name(), num);

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
    : backref(backref), type(type), num(num), date(date), uid(uid), pre_num(pre_num), description(description),
      cleanup(cleanup), userdata(userdata)
{
}


string
ProxySnapshotDbus::mountFilesystemSnapshot(bool user_request) const
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "MountSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name() << num << user_request;

    DBus::Message reply = conn().send_with_reply_and_block(call);

    string mount_point;

    DBus::Hihi hihi(reply);
    hihi >> mount_point;

    return mount_point;
}


void
ProxySnapshotDbus::umountFilesystemSnapshot(bool user_request) const
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "UmountSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name() << getNum() << user_request;

    conn().send_with_reply_and_block(call);
}


DBus::Connection&
ProxySnapshotDbus::conn() const
{
    return backref->conn();
}


const string
ProxySnapshotDbus::config_name() const
{
    return backref->config_name();
}


DBus::Connection&
ProxySnapshotsDbus::conn() const
{
    return backref->conn();
}


const string
ProxySnapshotsDbus::config_name() const
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
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "SetConfig");

    DBus::Hoho hoho(call);
    hoho << config_name << proxy_config.getAllValues();

    conn().send_with_reply_and_block(call);
}


void
ProxySnapperDbus::setupQuota()
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "SetupQuota");

    DBus::Hoho hoho(call);
    hoho << config_name;

    conn().send_with_reply_and_block(call);
}


void
ProxySnapperDbus::prepareQuota() const
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "PrepareQuota");

    DBus::Hoho hoho(call);
    hoho << config_name;

    conn().send_with_reply_and_block(call);
}


QuotaData
ProxySnapperDbus::queryQuotaData() const
{
    // TODO
}


ProxySnapshots::const_iterator
ProxySnapperDbus::createSingleSnapshot(const SCD& scd)
{
    unsigned int num = command_create_single_xsnapshot(conn(), config_name, scd.description,
						       scd.cleanup, scd.userdata);

    proxy_snapshots.emplace_back(new ProxySnapshotDbus(&proxy_snapshots, num));

    return --proxy_snapshots.end();
}


ProxySnapshots::const_iterator
ProxySnapperDbus::createPreSnapshot(const SCD& scd)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreatePreSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << scd.description << scd.cleanup << scd.userdata;

    DBus::Message reply = conn().send_with_reply_and_block(call);

    unsigned int num;

    DBus::Hihi hihi(reply);
    hihi >> num;

    proxy_snapshots.emplace_back(new ProxySnapshotDbus(&proxy_snapshots, num));

    return --proxy_snapshots.end();
}


ProxySnapshots::const_iterator
ProxySnapperDbus::createPostSnapshot(ProxySnapshots::const_iterator pre, const SCD& scd)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreatePostSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << pre->getNum() << scd.description << scd.cleanup << scd.userdata;

    DBus::Message reply = conn().send_with_reply_and_block(call);

    unsigned int num;

    DBus::Hihi hihi(reply);
    hihi >> num;

    proxy_snapshots.emplace_back(new ProxySnapshotDbus(&proxy_snapshots, num));

    return --proxy_snapshots.end();
}


void
ProxySnapperDbus::modifySnapshot(ProxySnapshots::iterator snapshot, const SMD& smd)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "SetSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << snapshot->getNum() << smd.description << smd.cleanup << smd.userdata;

    conn().send_with_reply_and_block(call);
}


void
ProxySnapperDbus::deleteSnapshots(list<ProxySnapshots::iterator> snapshots, bool verbose)
{
    list<unsigned int> nums;
    for (const ProxySnapshots::iterator& proxy_snapshot : snapshots)
	nums.push_back(proxy_snapshot->getNum());

    command_delete_xsnapshots(conn(), config_name, nums, verbose);

    // TODO remove from internal list
}


ProxyComparison
ProxySnapperDbus::createComparison(const ProxySnapshot& lhs, const ProxySnapshot& rhs, bool mount)
{
    return ProxyComparison(new ProxyComparisonDbus(this, lhs, rhs, mount));
}


void
ProxySnapperDbus::syncFilesystem() const
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "Sync");

    DBus::Hoho hoho(call);
    hoho << config_name;

    conn().send_with_reply_and_block(call);
}


void
ProxySnapshotsDbus::update()
{
    proxy_snapshots.clear();

    XSnapshots x = command_list_xsnapshots(conn(), config_name());
    for (XSnapshots::const_iterator it = x.begin(); it != x.end(); ++it)
	proxy_snapshots.push_back(new ProxySnapshotDbus(this, it->getType(), it->getNum(), it->getDate(),
							it->getUid(), it->getPreNum(), it->getDescription(),
							it->getCleanup(), it->getUserdata()));
}


ProxySnapshots&
ProxySnapperDbus::getSnapshots()
{
    proxy_snapshots.update();

    return proxy_snapshots;
}


DBus::Connection&
ProxySnapperDbus::conn() const
{
    return backref->conn;
}


void
ProxySnappersDbus::createConfig(const string& config_name, const string& subvolume,
				const string& fstype, const string& template_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreateConfig");

    DBus::Hoho hoho(call);
    hoho << config_name << subvolume << fstype << template_name;

    conn.send_with_reply_and_block(call);
}


void
ProxySnappersDbus::deleteConfig(const string& config_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "DeleteConfig");

    DBus::Hoho hoho(call);
    hoho << config_name;

    conn.send_with_reply_and_block(call);
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
    proxy_snappers.push_back(unique_ptr<ProxySnapperDbus>(ret));
    return ret;
}


map<string, ProxyConfig>
ProxySnappersDbus::getConfigs() const
{
   map<string, ProxyConfig> ret;

   list<XConfigInfo> config_infos = command_list_xconfigs(conn);
   for (XConfigInfo& x : config_infos)
       ret.emplace(make_pair(x.config_name, x.raw));

   return ret;
}


vector<string>
ProxySnappersDbus::debug() const
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "Debug");

    DBus::Message reply = conn.send_with_reply_and_block(call);

    vector<string> lines;

    DBus::Hihi hihi(reply);
    hihi >> lines;

    return lines;
}


ProxyComparisonDbus::ProxyComparisonDbus(ProxySnapperDbus* backref, const ProxySnapshot& lhs,
					 const ProxySnapshot& rhs, bool mount)
    : backref(backref), lhs(lhs), rhs(rhs), files(&file_paths)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreateComparison");

    DBus::Hoho hoho(call);
    hoho << backref->config_name << lhs.getNum() << rhs.getNum();

    backref->conn().send_with_reply_and_block(call);

    // TODO file_paths stuff

    if (mount)
    {
	if (!lhs.isCurrent())
	    file_paths.pre_path = command_mount_xsnapshots(backref->conn(), backref->config_name,
							   lhs.getNum(), false);
	else
	    file_paths.pre_path = file_paths.system_path;

	if (!rhs.isCurrent())
	    file_paths.post_path = command_mount_xsnapshots(backref->conn(), backref->config_name,
							    rhs.getNum(), false);
	else
	    file_paths.post_path = file_paths.system_path;
    }

    // TODO, use less tmps

    list<XFile> tmp1 = command_get_xfiles(backref->conn(), backref->config_name, lhs.getNum(),
					  rhs.getNum());

    vector<File> tmp2;

    for (list<XFile>::const_iterator it = tmp1.begin(); it != tmp1.end(); ++it)
    {
	File file(&file_paths, it->name, it->status);
	tmp2.push_back(file);
    }

    files = Files(&file_paths, tmp2);
}


ProxyComparisonDbus::~ProxyComparisonDbus()
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "DeleteComparison");

    DBus::Hoho hoho(call);
    hoho << backref->config_name << lhs.getNum() << rhs.getNum();

    backref->conn().send_with_reply_and_block(call);
}


ProxySnappers
ProxySnappers::createDbus()
{
    return ProxySnappers(new ProxySnappersDbus());
}
