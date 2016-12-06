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
    XSnapshot x = command_get_xsnapshot(*backref->backref->backref->conn,
					backref->backref->config_name, num);

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
    hoho << backref->backref->config_name << num << user_request;

    DBus::Message reply = backref->backref->backref->conn->send_with_reply_and_block(call);

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
    hoho << backref->backref->config_name << getNum() << user_request;

    backref->backref->backref->conn->send_with_reply_and_block(call);
}


void
ProxySnapperDbus::setConfigInfo(const map<string, string>& raw)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "SetConfig");

    DBus::Hoho hoho(call);
    hoho << config_name << raw;

    backref->conn->send_with_reply_and_block(call);
}


void
ProxySnapperDbus::setupQuota()
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "SetupQuota");

    DBus::Hoho hoho(call);
    hoho << config_name;

    backref->conn->send_with_reply_and_block(call);
}


void
ProxySnapperDbus::prepareQuota() const
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "PrepareQuota");

    DBus::Hoho hoho(call);
    hoho << config_name;

    backref->conn->send_with_reply_and_block(call);
}


ProxySnapshots::const_iterator
ProxySnapperDbus::createSingleSnapshot(const SCD& scd)
{
    unsigned int num = command_create_single_xsnapshot(*backref->conn, config_name, scd.description,
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

    DBus::Message reply = backref->conn->send_with_reply_and_block(call);

    unsigned int num;

    DBus::Hihi hihi(reply);
    hihi >> num;

    proxy_snapshots.emplace_back(new ProxySnapshotDbus(&proxy_snapshots, num));

    return --proxy_snapshots.end();
}


ProxySnapshots::const_iterator
ProxySnapperDbus::createPostSnapshot(const ProxySnapshots::const_iterator pre, const SCD& scd)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreatePostSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << pre->getNum() << scd.description << scd.cleanup << scd.userdata;

    DBus::Message reply = backref->conn->send_with_reply_and_block(call);

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

    backref->conn->send_with_reply_and_block(call);
}


void
ProxySnapperDbus::deleteSnapshots(list<ProxySnapshots::iterator> snapshots)
{
    list<unsigned int> nums;
    for (const ProxySnapshots::iterator& proxy_snapshot : snapshots)
	nums.push_back(proxy_snapshot->getNum());

    command_delete_xsnapshots(*backref->conn, config_name, nums, false);
}


void
ProxySnapperDbus::syncFilesystem() const
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "Sync");

    DBus::Hoho hoho(call);
    hoho << config_name;

    backref->conn->send_with_reply_and_block(call);
}


void
ProxySnapshotsDbus::update()
{
    proxy_snapshots.clear();

    XSnapshots x = command_list_xsnapshots(*backref->backref->conn, backref->config_name);
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


void
ProxySnappersDbus::createConfig(const string& config_name, const string& subvolume,
				const string& fstype, const string& template_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreateConfig");

    DBus::Hoho hoho(call);
    hoho << config_name << subvolume << fstype << template_name;

    conn->send_with_reply_and_block(call);
}


void
ProxySnappersDbus::deleteConfig(const string& config_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "DeleteConfig");

    DBus::Hoho hoho(call);
    hoho << config_name;

    conn->send_with_reply_and_block(call);
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


vector<string>
ProxySnappersDbus::debug()
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "Debug");

    DBus::Message reply = conn->send_with_reply_and_block(call);

    vector<string> lines;

    DBus::Hihi hihi(reply);
    hihi >> lines;

    return lines;
}
