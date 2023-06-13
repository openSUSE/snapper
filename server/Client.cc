/*
 * Copyright (c) [2012-2015] Novell, Inc.
 * Copyright (c) [2016-2023] SUSE LLC
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

#include <snapper/Log.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/AsciiFile.h>
#include <snapper/Version.h>
#include <dbus/DBusMessage.h>
#include <dbus/DBusConnection.h>

#include "Types.h"
#include "Client.h"
#include "MetaSnapper.h"
#include "Background.h"


boost::shared_mutex big_mutex;


Client::Client(const string& name, uid_t uid, const Clients& clients)
    : name(name), uid(uid), clients(clients)
{
}


Client::~Client()
{
    method_call_thread.interrupt();
    files_transfer_thread.interrupt();

    if (method_call_thread.joinable())
	method_call_thread.join();

    if (files_transfer_thread.joinable())
	files_transfer_thread.join();

    for (list<Comparison*>::iterator it = comparisons.begin(); it != comparisons.end(); ++it)
    {
	delete_comparison(it);
    }

    for (map<pair<string, unsigned int>, unsigned int>::iterator it1 = mounts.begin();
	 it1 != mounts.end(); ++it1)
    {
	string config_name = it1->first.first;
	unsigned int number = it1->first.second;
	unsigned int use_count = it1->second;

	MetaSnappers::iterator it2 = meta_snappers.find(config_name);

	Snapper* snapper = it2->getSnapper();
	Snapshots& snapshots = snapper->getSnapshots();

	Snapshots::iterator snap = snapshots.find(number);
	if (snap != snapshots.end())
	    for (unsigned int i = 0; i < use_count; ++i)
		snap->umountFilesystemSnapshot(false);
    }
}


list<Comparison*>::iterator
Client::find_comparison(Snapper* snapper, Snapshots::const_iterator snapshot1,
			Snapshots::const_iterator snapshot2)
{
    for (list<Comparison*>::iterator it = comparisons.begin(); it != comparisons.end(); ++it)
    {
	if ((*it)->getSnapper() == snapper && (*it)->getSnapshot1() == snapshot1 &&
	    (*it)->getSnapshot2() == snapshot2)
	    return it;
    }

    SN_THROW(NoComparison());
    __builtin_unreachable();
}


list<Comparison*>::iterator
Client::find_comparison(Snapper* snapper, unsigned int number1, unsigned int number2)
{
    Snapshots& snapshots = snapper->getSnapshots();
    Snapshots::const_iterator snapshot1 = snapshots.find(number1);
    Snapshots::const_iterator snapshot2 = snapshots.find(number2);

    return find_comparison(snapper, snapshot1, snapshot2);
}


void
Client::delete_comparison(list<Comparison*>::iterator it)
{
    const Snapper* s = (*it)->getSnapper();

    for (MetaSnappers::iterator it2 = meta_snappers.begin(); it2 != meta_snappers.end(); ++it2)
    {
	if (it2->is_equal(s))
	    it2->dec_use_count();
    }

    delete *it;
    *it = nullptr;
}


void
Client::add_lock(const string& config_name)
{
    locks.insert(config_name);
}


void
Client::remove_lock(const string& config_name)
{
    locks.erase(config_name);
}


bool
Client::has_lock(const string& config_name) const
{
    return contains(locks, config_name);
}


void
Client::add_mount(const string& config_name, unsigned int number)
{
    mounts[make_pair(config_name, number)]++;
}


void
Client::remove_mount(const string& config_name, unsigned int number)
{
    map<pair<string, unsigned int>, unsigned int>::iterator it =
	mounts.find(make_pair(config_name, number));
    if (it != mounts.end())
    {
	if (it->second > 1)
	    it->second--;
	else
	    mounts.erase(it);
    }
}


void
Client::introspect(DBus::Connection& conn, DBus::Message& msg)
{
    const char* introspect =
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE "\n"
	"<node name='/org/opensuse/Snapper'>\n"
	"  <interface name='" DBUS_INTERFACE_INTROSPECTABLE "'>\n"
	"    <method name='Introspect'>\n"
	"      <arg name='xml_data' type='s' direction='out'/>\n"
	"    </method>\n"
	"  </interface>\n"
	"  <interface name='" INTERFACE "'>\n"

	"    <signal name='ConfigCreated'>\n"
	"      <arg name='config-name' type='s'/>\n"
	"    </signal>\n"

	"    <signal name='ConfigModified'>\n"
	"      <arg name='config-name' type='s'/>\n"
	"    </signal>\n"

	"    <signal name='ConfigDeleted'>\n"
	"      <arg name='config-name' type='s'/>\n"
	"    </signal>\n"

	"    <signal name='SnapshotCreated'>\n"
	"      <arg name='config-name' type='s'/>\n"
	"      <arg name='number' type='u'/>\n"
	"    </signal>\n"

	"    <signal name='SnapshotModified'>\n"
	"      <arg name='config-name' type='s'/>\n"
	"      <arg name='number' type='u'/>\n"
	"    </signal>\n"

	"    <signal name='SnapshotsDeleted'>\n"
	"      <arg name='config-name' type='s'/>\n"
	"      <arg name='number' type='u'/>\n"
	"    </signal>\n"

	"    <method name='ListConfigs'>\n"
	"      <arg name='configs' type='a(ssa{ss})' direction='out'/>\n"
	"    </method>\n"

	"    <method name='GetConfig'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='data' type='(ssa{ss})' direction='out'/>\n"
	"    </method>\n"

	"    <method name='SetConfig'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='data' type='a{ss}' direction='in'/>\n"
	"    </method>\n"

	"    <method name='CreateConfig'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='subvolume' type='s' direction='in'/>\n"
	"      <arg name='fstype' type='s' direction='in'/>\n"
	"      <arg name='template-name' type='s' direction='in'/>\n"
	"    </method>\n"

	"    <method name='DeleteConfig'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"    </method>\n"

	"    <method name='LockConfig'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"    </method>\n"

	"    <method name='UnlockConfig'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"    </method>\n"

	"    <method name='ListSnapshots'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='snapshots' type='a(uquxussa{ss})' direction='out'/>\n"
	"    </method>\n"

	"    <method name='ListSnapshotsAtTime'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='begin' type='x' direction='in'/>\n"
	"      <arg name='end' type='x' direction='in'/>\n"
	"      <arg name='snapshots' type='a(uquxussa{ss})' direction='out'/>\n"
	"    </method>\n"

	"    <method name='GetSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='type' type='(uquxussa{ss})' direction='out'/>\n"
	"    </method>\n"

	"    <method name='SetSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='description' type='s' direction='in'/>\n"
	"      <arg name='cleanup' type='s' direction='in'/>\n"
	"      <arg name='userdata' type='a{ss}' direction='in'/>\n"
	"    </method>\n"

	"    <method name='CreateSingleSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='description' type='s' direction='in'/>\n"
	"      <arg name='cleanup' type='s' direction='in'/>\n"
	"      <arg name='userdata' type='a{ss}' direction='in'/>\n"
	"      <arg name='number' type='u' direction='out'/>\n"
	"    </method>\n"

	"    <method name='CreateSingleSnapshotV2'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='parent-number' type='u' direction='in'/>\n"
	"      <arg name='read-only' type='b' direction='in'/>\n"
	"      <arg name='description' type='s' direction='in'/>\n"
	"      <arg name='cleanup' type='s' direction='in'/>\n"
	"      <arg name='userdata' type='a{ss}' direction='in'/>\n"
	"      <arg name='number' type='u' direction='out'/>\n"
	"    </method>\n"

	"    <method name='CreateSingleSnapshotOfDefault'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='read-only' type='b' direction='in'/>\n"
	"      <arg name='description' type='s' direction='in'/>\n"
	"      <arg name='cleanup' type='s' direction='in'/>\n"
	"      <arg name='userdata' type='a{ss}' direction='in'/>\n"
	"      <arg name='number' type='u' direction='out'/>\n"
	"    </method>\n"

	"    <method name='CreatePreSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='description' type='s' direction='in'/>\n"
	"      <arg name='cleanup' type='s' direction='in'/>\n"
	"      <arg name='userdata' type='a{ss}' direction='in'/>\n"
	"      <arg name='number' type='u' direction='out'/>\n"
	"    </method>\n"

	"    <method name='CreatePostSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='pre-number' type='u' direction='in'/>\n"
	"      <arg name='description' type='s' direction='in'/>\n"
	"      <arg name='cleanup' type='s' direction='in'/>\n"
	"      <arg name='userdata' type='a{ss}' direction='in'/>\n"
	"      <arg name='number' type='u' direction='out'/>\n"
	"    </method>\n"

	"    <method name='DeleteSnapshots'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='numbers' type='au' direction='in'/>\n"
	"    </method>\n"

	"    <method name='IsSnapshotReadOnly'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='read-only' type='b' direction='out'/>\n"
	"    </method>\n"

	"    <method name='SetSnapshotReadOnly'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='read-only' type='b' direction='in'/>\n"
	"    </method>\n"

	"    <method name='GetDefaultSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='valid' type='b' direction='out'/>\n"
	"      <arg name='number' type='u' direction='out'/>\n"
	"    </method>\n"

	"    <method name='GetActiveSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='valid' type='b' direction='out'/>\n"
	"      <arg name='number' type='u' direction='out'/>\n"
	"    </method>\n"

	"    <method name='CalculateUsedSpace'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"    </method>\n"

	"    <method name='GetUsedSpace'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='sued-space' type='u' direction='out'/>\n"
	"    </method>\n"

	"    <method name='MountSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='user-request' type='b' direction='in'/>\n"
	"      <arg name='path' type='s' direction='out'/>\n"
	"    </method>\n"

	"    <method name='UmountSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='user-request' type='b' direction='in'/>\n"
	"    </method>\n"

	"    <method name='GetMountPoint'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='path' type='s' direction='out'/>\n"
	"    </method>\n"

	"    <method name='CreateComparison'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
	"      <arg name='num-files' type='u' direction='out'/>\n"
	"    </method>\n"

	"    <method name='DeleteComparison'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
	"    </method>\n"

	"    <method name='GetFiles'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
	"      <arg name='files' type='a(su)' direction='out'/>\n"
	"    </method>\n"

	"    <method name='GetFilesByPipe'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
	"      <arg name='fd' type='h' direction='out'/>\n"
	"    </method>\n"

	"    <method name='Sync'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"    </method>\n"

	"  </interface>\n"
	"</node>\n";

    y2deb("Introspect");

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << introspect;

    conn.send(reply);
}


struct Permissions : public Exception
{
    explicit Permissions() : Exception("no permissions") {}
};


void
Client::check_permission(DBus::Connection& conn, DBus::Message& msg) const
{
    // Check if the uid of the dbus-user is root.
    if (uid == 0)
	return;

    SN_THROW(Permissions());
}


void
Client::check_permission(DBus::Connection& conn, DBus::Message& msg,
			 const MetaSnapper& meta_snapper) const
{
    // Check if the uid of the dbus-user is root.
    if (uid == 0)
	return;

    // Check if the uid of the dbus-user is included in the allowed uids.
    if (contains(meta_snapper.get_allowed_uids(), uid))
	return;

    string username;
    gid_t gid;

    if (get_uid_username_gid(uid, username, gid))
    {
	// Check if the primary gid of the dbus-user is included in the allowed gids.
	if (contains(meta_snapper.get_allowed_gids(), gid))
	    return;

	vector<gid_t> gids = getgrouplist(username.c_str(), gid);

	// Check if any (primary or secondary) gid of the dbus-user is included in the allowed
	// gids.
	for (vector<gid_t>::const_iterator it = gids.begin(); it != gids.end(); ++it)
	    if (contains(meta_snapper.get_allowed_gids(), *it))
		return;
    }

    SN_THROW(Permissions());
}


struct Lock : public Exception
{
    explicit Lock() : Exception("locked") {}
};


void
Client::check_lock(DBus::Connection& conn, DBus::Message& msg, const string& config_name) const
{
    for (Clients::const_iterator it = clients.begin(); it != clients.end(); ++it)
    {
	if (it->zombie || &*it == this)
	    continue;

	if (it->has_lock(config_name))
	    SN_THROW(Lock());
    }
}


struct ConfigInUse : public Exception
{
    explicit ConfigInUse() : Exception("config in use") {}
};


struct SnapshotInUse : public Exception
{
    explicit SnapshotInUse() : Exception("snapshot in use") {}
};


void
Client::check_config_in_use(const MetaSnapper& meta_snapper) const
{
    if (meta_snapper.use_count() != 0)
	SN_THROW(ConfigInUse());
}


void
Client::check_snapshot_in_use(const MetaSnapper& meta_snapper, unsigned int number) const
{
    for (Clients::const_iterator it1 = clients.begin(); it1 != clients.end(); ++it1)
    {
	map<pair<string, unsigned int>, unsigned int>::const_iterator it2 =
	    it1->mounts.find(make_pair(meta_snapper.configName(), number));

	if (it2 != it1->mounts.end())
	    SN_THROW(SnapshotInUse());
    }
}


void
Client::signal_config_created(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "ConfigCreated");

    DBus::Marshaller marshaller(msg);
    marshaller << config_name;

    conn.send(msg);
}


void
Client::signal_config_modified(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "ConfigModified");

    DBus::Marshaller marshaller(msg);
    marshaller << config_name;

    conn.send(msg);
}


void
Client::signal_config_deleted(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "ConfigDeleted");

    DBus::Marshaller marshaller(msg);
    marshaller << config_name;

    conn.send(msg);
}


void
Client::signal_snapshot_created(DBus::Connection& conn, const string& config_name,
				unsigned int num)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "SnapshotCreated");

    DBus::Marshaller marshaller(msg);
    marshaller << config_name << num;

    conn.send(msg);
}


void
Client::signal_snapshot_modified(DBus::Connection& conn, const string& config_name,
				 unsigned int num)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "SnapshotModified");

    DBus::Marshaller marshaller(msg);
    marshaller << config_name << num;

    conn.send(msg);
}


void
Client::signal_snapshots_deleted(DBus::Connection& conn, const string& config_name,
				 const vector<dbus_uint32_t>& nums)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "SnapshotsDeleted");

    DBus::Marshaller marshaller(msg);
    marshaller << config_name << nums;

    conn.send(msg);
}


void
Client::list_configs(DBus::Connection& conn, DBus::Message& msg)
{
    y2deb("ListConfigs");

    boost::shared_lock<boost::shared_mutex> lock(big_mutex);

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller.open_array(DBus::TypeInfo<ConfigInfo>::signature);
    for (MetaSnappers::const_iterator it = meta_snappers.begin(); it != meta_snappers.end(); ++it)
	marshaller << it->getConfigInfo();
    marshaller.close_array();

    conn.send(reply);
}


void
Client::get_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("GetConfig config_name:" << config_name);

    boost::shared_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::const_iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << it->getConfigInfo();

    conn.send(reply);
}


void
Client::set_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    map<string, string> raw;
    unmarshaller >> config_name >> raw;

    y2deb("SetConfig config_name:" << config_name << " raw:" << raw);

    boost::shared_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg);

    it->setConfigInfo(raw);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);

    signal_config_modified(conn, config_name);
}


void
Client::create_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    string subvolume;
    string fstype;
    string template_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> subvolume >> fstype >> template_name;

    y2deb("CreateConfig config_name:" << config_name << " subvolume:" << subvolume <<
	  " fstype:" << fstype << " template_name:" << template_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    check_permission(conn, msg);

    meta_snappers.createConfig(config_name, subvolume, fstype, template_name);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);

    signal_config_created(conn, config_name);
}


void
Client::delete_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("DeleteConfig config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg);
    check_lock(conn, msg, config_name);
    check_config_in_use(*it);

    meta_snappers.deleteConfig(it);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);

    signal_config_deleted(conn, config_name);
}


void
Client::lock_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("LockConfig config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    add_lock(config_name);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::unlock_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("UnlockConfig config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    remove_lock(config_name);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::list_snapshots(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("ListSnapshots config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << snapshots;

    conn.send(reply);
}


void
Client::list_snapshots_at_time(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    time_t begin, end;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> begin >> end;

    y2deb("ListSnapshotsAtTime config_name:" << config_name << " begin:" << begin <<
	  " end:" << end);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);

    marshaller.open_array(DBus::TypeInfo<Snapshot>::signature);
    for (Snapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
    {
	if (it->getDate() >= begin && it->getDate() <= end)
	    marshaller << *it;
    }
    marshaller.close_array();

    conn.send(reply);
}


void
Client::get_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num;

    y2deb("GetSnapshot config_name:" << config_name << " num:" << num);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	SN_THROW(IllegalSnapshotException());

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << *snap;

    conn.send(reply);
}


void
Client::set_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;
    SMD smd;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num >> smd.description >> smd.cleanup >> smd.userdata;

    y2deb("SetSnapshot config_name:" << config_name << " num:" << num);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	SN_THROW(IllegalSnapshotException());

    snapper->modifySnapshot(snap, smd);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);

    signal_snapshot_modified(conn, config_name, snap->getNum());
}


void
Client::create_single_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    SCD scd;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> scd.description >> scd.cleanup >> scd.userdata;

    y2deb("CreateSingleSnapshot config_name:" << config_name << " description:" << scd.description <<
	  " cleanup:" << scd.cleanup);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);
    scd.uid = uid;

    Snapper* snapper = it->getSnapper();

    Snapshots::iterator snap1 = snapper->createSingleSnapshot(scd);

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << snap1->getNum();

    conn.send(reply);

    signal_snapshot_created(conn, config_name, snap1->getNum());
}


void
Client::create_single_snapshot_v2(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    unsigned int parent_num;
    SCD scd;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> parent_num >> scd.read_only >> scd.description >> scd.cleanup >> scd.userdata;

    y2deb("CreateSingleSnapshotV2 config_name:" << config_name << " parent_num:" << parent_num <<
	  " read_only:" << scd.read_only << " description:" << scd.description << " cleanup:" << scd.cleanup);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);
    scd.uid = uid;

    Snapper* snapper = it->getSnapper();

    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator parent = snapshots.find(parent_num);

    Snapshots::iterator snap2 = snapper->createSingleSnapshot(parent, scd);

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << snap2->getNum();

    conn.send(reply);

    signal_snapshot_created(conn, config_name, snap2->getNum());
}


void
Client::create_single_snapshot_of_default(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    SCD scd;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> scd.read_only >> scd.description >> scd.cleanup >> scd.userdata;

    y2deb("CreateSingleSnapshotOfDefault config_name:" << config_name << " read_only:" <<
	  scd.read_only << " description:" << scd.description << " cleanup:" << scd.cleanup);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);
    scd.uid = uid;

    Snapper* snapper = it->getSnapper();

    Snapshots::iterator snap = snapper->createSingleSnapshotOfDefault(scd);

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << snap->getNum();

    conn.send(reply);

    signal_snapshot_created(conn, config_name, snap->getNum());
}


void
Client::create_pre_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    SCD scd;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> scd.description >> scd.cleanup >> scd.userdata;

    y2deb("CreatePreSnapshot config_name:" << config_name << " description:" << scd.description <<
	  " cleanup:" << scd.cleanup);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);
    scd.uid = uid;

    Snapper* snapper = it->getSnapper();

    Snapshots::iterator snap1 = snapper->createPreSnapshot(scd);

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << snap1->getNum();

    conn.send(reply);

    signal_snapshot_created(conn, config_name, snap1->getNum());
}


void
Client::create_post_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    unsigned int pre_num;
    SCD scd;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> pre_num >> scd.description >> scd.cleanup >> scd.userdata;

    y2deb("CreatePostSnapshot config_name:" << config_name << " pre_num:" << pre_num <<
	  " description:" << scd.description << " cleanup:" << scd.cleanup);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);
    scd.uid = uid;

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap1 = snapshots.find(pre_num);

    Snapshots::iterator snap2 = snapper->createPostSnapshot(snap1, scd);

    bool background_comparison = true;
    it->getConfigInfo().get_value("BACKGROUND_COMPARISON", background_comparison);
    if (background_comparison)
	clients.backgrounds().add_task(it, snap1, snap2);

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << snap2->getNum();

    conn.send(reply);

    signal_snapshot_created(conn, config_name, snap2->getNum());
}


void
Client::delete_snapshots(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    vector<dbus_uint32_t> nums;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> nums;

    y2deb("DeleteSnapshots config_name:" << config_name << " nums:" << nums);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it1 = meta_snappers.find(config_name);

    check_permission(conn, msg, *it1);
    check_lock(conn, msg, config_name);
    check_config_in_use(*it1);

    Snapper* snapper = it1->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    for (vector<unsigned int>::const_iterator it2 = nums.begin(); it2 != nums.end(); ++it2)
    {
	check_snapshot_in_use(*it1, *it2);

	Snapshots::iterator snap = snapshots.find(*it2);

	snapper->deleteSnapshot(snap);
    }

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);

    signal_snapshots_deleted(conn, config_name, nums);
}


void
Client::is_snapshot_read_only(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num;

    y2deb("IsSnapshotReadOnly config_name:" << config_name << " num:" << num);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	SN_THROW(IllegalSnapshotException());

    bool read_only = snap->isReadOnly();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << read_only;

    conn.send(reply);
}


void
Client::set_snapshot_read_only(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;
    bool read_only;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num >> read_only;

    y2deb("SetSnapshotReadOnly config_name:" << config_name << " num:" << num << " read_only:" << read_only);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	SN_THROW(IllegalSnapshotException());

    snap->setReadOnly(read_only);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::get_default_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("GetDefaultSnapshot config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::const_iterator tmp = snapshots.getDefault();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);

    if (tmp != snapshots.end())
	marshaller << true << tmp->getNum();
    else
	marshaller << false << (unsigned int)(0);

    conn.send(reply);
}


void
Client::get_active_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("GetActiveSnapshot config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::const_iterator tmp = snapshots.getActive();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);

    if (tmp != snapshots.end())
	marshaller << true << tmp->getNum();
    else
	marshaller << false << (unsigned int)(0);

    conn.send(reply);
}


void
Client::calculate_used_space(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("CalculateUsedSpace config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    snapper->calculateUsedSpace();

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::get_used_space(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num;

    y2deb("GetUsedSpace config_name:" << config_name << " num:" << num);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	SN_THROW(IllegalSnapshotException());

    uint64_t used_space = snap->getUsedSpace();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << used_space;

    conn.send(reply);
}


void
Client::mount_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;
    bool user_request;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num >> user_request;

    y2deb("MountSnapshot config_name:" << config_name << " num:" << num <<
	  " user_request:" << user_request);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	SN_THROW(IllegalSnapshotException());

    snap->mountFilesystemSnapshot(user_request);

    if (!user_request)
	add_mount(config_name, num);

    string mount_point = snap->snapshotDir();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << mount_point;

    conn.send(reply);
}


void
Client::umount_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;
    bool user_request;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num >> user_request;

    y2deb("UmountSnapshot config_name:" << config_name << " num:" << num <<
	  " user_request:" << user_request);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	SN_THROW(IllegalSnapshotException());

    snap->umountFilesystemSnapshot(user_request);

    if (!user_request)
	remove_mount(config_name, num);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::get_mount_point(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num;

    y2deb("GetMountPoint config_name:" << config_name << " num:" << num);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();
    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	SN_THROW(IllegalSnapshotException());

    string mount_point = snap->snapshotDir();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << mount_point;

    conn.send(reply);
}


void
Client::create_comparison(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num1 >> num2;

    y2deb("CreateComparison config_name:" << config_name << " num1:" << num1 << " num2:" << num2);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();
    Snapshots::const_iterator snapshot1 = snapshots.find(num1);
    Snapshots::const_iterator snapshot2 = snapshots.find(num2);

    RefHolder ref_holder(*it);

    lock.unlock();

    Comparison* comparison = new Comparison(snapper, snapshot1, snapshot2, false);

    lock.lock();

    comparisons.push_back(comparison);

    it->inc_use_count();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    dbus_uint32_t num_files = comparison->getFiles().size();
    marshaller << num_files;

    conn.send(reply);
}


void
Client::delete_comparison(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num1 >> num2;

    y2deb("DeleteComparison config_name:" << config_name << " num1:" << num1 << " num2:" << num2);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    list<Comparison*>::iterator it2 = find_comparison(it->getSnapper(), num1, num2);

    delete_comparison(it2);
    comparisons.erase(it2);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::get_files(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num1 >> num2;

    y2deb("GetFiles config_name:" << config_name << " num1:" << num1 << " num2:" << num2);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    list<Comparison*>::iterator it2 = find_comparison(it->getSnapper(), num1, num2);

    const Files& files = (*it2)->getFiles();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << files;

    conn.send(reply);
}


void
Client::get_files_by_pipe(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name >> num1 >> num2;

    y2deb("GetFilesByPipe config_name:" << config_name << " num1:" << num1 << " num2:" << num2);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    list<Comparison*>::iterator it2 = find_comparison(it->getSnapper(), num1, num2);

    const Files& files = (*it2)->getFiles();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);

    shared_ptr<FilesTransferTask> files_transfer_task = make_shared<FilesTransferTask>(files);

    marshaller << files_transfer_task->get_read_end();
    conn.send(reply);

    files_transfer_task->get_read_end().close();

    add_files_transfer_task(files_transfer_task);
}


void
Client::setup_quota(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("SetupQuota config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    snapper->setupQuota();
    it->updateConfigInfo("QGROUP");

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::prepare_quota(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("PrepareQuota config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    snapper->prepareQuota();

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::query_quota(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("QueryQuota config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    QuotaData quota_data = snapper->queryQuotaData();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << quota_data;

    conn.send(reply);
}


void
Client::query_free_space(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("QueryFreeSpace config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    FreeSpaceData free_space_data = snapper->queryFreeSpaceData();

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);
    marshaller << free_space_data;

    conn.send(reply);
}


void
Client::sync(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Unmarshaller unmarshaller(msg);
    unmarshaller >> config_name;

    y2deb("Sync config_name:" << config_name);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    snapper->syncFilesystem();

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::debug(DBus::Connection& conn, DBus::Message& msg)
{
    y2deb("Debug");

    boost::shared_lock<boost::shared_mutex> lock(big_mutex);

    check_permission(conn, msg);

    DBus::MessageMethodReturn reply(msg);

    DBus::Marshaller marshaller(reply);

    marshaller.open_array("s");

    marshaller << "server:";
    {
	std::ostringstream s;
	s << "    pid:" << getpid();
	marshaller << s.str();
    }

    marshaller << "clients:";
    for (Clients::const_iterator it = clients.begin(); it != clients.end(); ++it)
    {
	std::ostringstream s;
	s << "    name:'" << it->name << "', uid:" << it->uid;
	if (&*it == this)
	    s << ", myself";
	if (it->zombie)
	    s << ", zombie";
	if (!it->locks.empty())
	    s << ", locks " << it->locks.size();
	if (!it->comparisons.empty())
	    s << ", comparisons " << it->comparisons.size();
	marshaller << s.str();
    }

    marshaller << "backgrounds:";
    for (Backgrounds::const_iterator it = clients.backgrounds().begin(); it != clients.backgrounds().end(); ++it)
    {
	std::ostringstream s;
	s << "    name:'" << it->meta_snapper->configName() << "'";
	marshaller << s.str();
    }

    marshaller << "meta-snappers:";
    for (MetaSnappers::const_iterator it = meta_snappers.begin(); it != meta_snappers.end(); ++it)
    {
	std::ostringstream s;
	s << "    name:'" << it->configName() << "'";
	if (it->is_loaded())
	{
	    s << ", loaded";
	    if (it->use_count() == 0)
		s << ", unused for " << duration_cast<milliseconds>(it->unused_for()).count() << "ms";
	    else
		s << ", use count " << it->use_count();
	}
	marshaller << s.str();
    }

    marshaller << "compile options:";
    marshaller << "    version " + string(Snapper::compileVersion());
    marshaller << "    libversion " LIBSNAPPER_MAJOR "." LIBSNAPPER_MINOR "." LIBSNAPPER_PATCHLEVEL;
    marshaller << "    flags " + string(Snapper::compileFlags());

    marshaller.close_array();

    conn.send(reply);
}


void
Client::dispatch(DBus::Connection& conn, DBus::Message& msg)
{
    using method_fnc = void (Client ::*)(DBus::Connection& conn, DBus::Message& msg);

    static const vector<pair<const char*, method_fnc>> method_registry = {
	{ "ListConfigs", &Client::list_configs },
	{ "CreateConfig", &Client::create_config },
	{ "GetConfig", &Client::get_config },
	{ "SetConfig", &Client::set_config },
	{ "DeleteConfig", &Client::delete_config },
	{ "LockConfig", &Client::lock_config },
	{ "UnlockConfig", &Client::unlock_config },
	{ "ListSnapshots", &Client::list_snapshots },
	{ "ListSnapshotsAtTime", &Client::list_snapshots_at_time },
	{ "GetSnapshot", &Client::get_snapshot },
	{ "SetSnapshot", &Client::set_snapshot },
	{ "CreateSingleSnapshot", &Client::create_single_snapshot },
	{ "CreateSingleSnapshotV2", &Client::create_single_snapshot_v2 },
	{ "CreateSingleSnapshotOfDefault", &Client::create_single_snapshot_of_default },
	{ "CreatePreSnapshot", &Client::create_pre_snapshot },
	{ "CreatePostSnapshot", &Client::create_post_snapshot },
	{ "DeleteSnapshots", &Client::delete_snapshots },
	{ "IsSnapshotReadOnly", &Client::is_snapshot_read_only },
	{ "SetSnapshotReadOnly", &Client::set_snapshot_read_only },
	{ "GetDefaultSnapshot", &Client::get_default_snapshot },
	{ "GetActiveSnapshot", &Client::get_active_snapshot },
	{ "CalculateUsedSpace", &Client::calculate_used_space },
	{ "GetUsedSpace", &Client::get_used_space },
	{ "MountSnapshot", &Client::mount_snapshot },
	{ "UmountSnapshot", &Client::umount_snapshot },
	{ "GetMountPoint", &Client::get_mount_point },
	{ "CreateComparison", &Client::create_comparison },
	{ "DeleteComparison", &Client::delete_comparison },
	{ "GetFiles", &Client::get_files },
	{ "GetFilesByPipe", &Client::get_files_by_pipe },
	{ "SetupQuota", &Client::setup_quota },
	{ "PrepareQuota", &Client::prepare_quota },
	{ "QueryQuota", &Client::query_quota },
	{ "QueryFreeSpace", &Client::query_free_space },
	{ "Sync", &Client::sync },
	{ "Debug", &Client::debug }
    };

    try
    {
	for (const vector<pair<const char*, method_fnc>>::value_type& tmp : method_registry)
	{
	    if (msg.is_method_call(INTERFACE, tmp.first))
	    {
		(*this.*tmp.second)(conn, msg);
		return;
	    }
	}

	DBus::MessageError reply(msg, "error.unknown_method", DBUS_ERROR_FAILED);
	conn.send(reply);
	return;
    }
    catch (const boost::thread_interrupted&)
    {
	throw;
    }
    catch (const DBus::MarshallingException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.dbus.marshalling", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const DBus::FatalException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.dbus.fatal", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const UnknownConfig& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.unknown_config", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const ListConfigsFailedException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.list_configs_failed", e.what());
	conn.send(reply);
    }
    catch (const CreateConfigFailedException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.create_config_failed", e.what());
	conn.send(reply);
    }
    catch (const DeleteConfigFailedException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.delete_config_failed", e.what());
	conn.send(reply);
    }
    catch (const Permissions& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.no_permissions", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const Lock& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.config_locked", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const ConfigInUse& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.config_in_use", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const SnapshotInUse& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.snapshot_in_use", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const NoComparison& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.no_comparisons", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const IllegalSnapshotException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.illegal_snapshot", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const CreateSnapshotFailedException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.create_snapshot_failed", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const DeleteSnapshotFailedException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.delete_snapshot_failed", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const InvalidConfigdataException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.invalid_configdata", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const InvalidUserdataException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.invalid_userdata", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const AclException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.acl_error", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const IOErrorException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.io_error", e.what());
	conn.send(reply);
    }
    catch (const IsSnapshotMountedFailedException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.is_snapshot_mounted", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const MountSnapshotFailedException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.mount_snapshot", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const UmountSnapshotFailedException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.umount_snapshot", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const InvalidUserException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.invalid_user", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const InvalidGroupException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.invalid_group", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const QuotaException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.quota", e.what());
	conn.send(reply);
    }
    catch (const FreeSpaceException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.free_space", e.what());
	conn.send(reply);
    }
    catch (const UnsupportedException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.unsupported", e.what());
	conn.send(reply);
    }
    catch (const StreamException& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.stream", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const Exception& e)
    {
	SN_CAUGHT(e);
	DBus::MessageError reply(msg, "error.something", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const exception& e)
    {
	y2err("caught unknown exception (" << e.what() << ")");
	DBus::MessageError reply(msg, "error.something", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (...)
    {
	y2err("caught unknown exception");
	DBus::MessageError reply(msg, "error.something", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
}


void
Client::add_method_call_task(DBus::Connection& conn, DBus::Message& msg)
{
    if (method_call_thread.get_id() == boost::thread::id())
	method_call_thread = boost::thread(boost::bind(&Client::method_call_worker, this));

    boost::unique_lock<boost::mutex> lock(method_call_mutex);
    method_call_tasks.push(MethodCallTask(conn, msg));
    lock.unlock();

    method_call_condition.notify_one();
}


void
Client::add_files_transfer_task(shared_ptr<FilesTransferTask> files_transfer_task)
{
    if (files_transfer_thread.get_id() == boost::thread::id())
	files_transfer_thread = boost::thread(boost::bind(&Client::files_transfer_worker, this));

    boost::unique_lock<boost::mutex> lock(files_transfer_mutex);
    files_transfer_tasks.push(files_transfer_task);
    lock.unlock();

    files_transfer_condition.notify_one();
}


void
Client::method_call_worker()
{
    try
    {
	while (true)
	{
	    boost::unique_lock<boost::mutex> lock(method_call_mutex);
	    while (method_call_tasks.empty())
		method_call_condition.wait(lock);
	    MethodCallTask method_call_task = method_call_tasks.front();
	    method_call_tasks.pop();
	    lock.unlock();

	    dispatch(method_call_task.conn, method_call_task.msg);
	}
    }
    catch (const boost::thread_interrupted&)
    {
	y2deb("worker interrupted");
    }
}


Clients::Clients(Backgrounds& backgrounds)
    : bgs(backgrounds)
{
}


Backgrounds&
Clients::backgrounds() const
{
    return bgs;
}


Clients::iterator
Clients::find(const string& name)
{
    for (iterator it = entries.begin(); it != entries.end(); ++it)
	if (it->name == name)
	    return it;

    return entries.end();
}


Clients::iterator
Clients::add(const string& name, uid_t uid)
{
    assert(find(name) == entries.end());

    entries.emplace_back(name, uid, *this);

    return --entries.end();
}


void
Clients::remove_zombies()
{
    for (iterator it = begin(); it != end();)
    {
	if (it->zombie && it->method_call_thread.timed_join(boost::posix_time::seconds(0)))
	    it = entries.erase(it);
	else
	    ++it;
    }
}


bool
Clients::has_zombies() const
{
    for (const_iterator it = begin(); it != end(); ++it)
	if (it->zombie)
	    return true;

    return false;
}


void
Client::files_transfer_worker()
{
    try
    {
	while (true)
	{
	    boost::unique_lock<boost::mutex> lock(files_transfer_mutex);
	    while (files_transfer_tasks.empty())
		files_transfer_condition.wait(lock);

	    shared_ptr<FilesTransferTask> ptr(files_transfer_tasks.front());
	    files_transfer_tasks.pop();
	    lock.unlock();

	    try
	    {
		ptr->run();
	    }
	    catch (const StreamException& e)
	    {
		SN_CAUGHT(e);
		y2err("error occured during files transfer");
	    }
	}
    }
    catch (const boost::thread_interrupted&)
    {
	y2deb("files transfer worker interrupted");
    }
}
