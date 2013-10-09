/*
 * Copyright (c) [2012-2013] Novell, Inc.
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


#include <snapper/Log.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/AsciiFile.h>
#include <dbus/DBusMessage.h>
#include <dbus/DBusConnection.h>

#include "Types.h"
#include "Client.h"
#include "MetaSnapper.h"
#include "Background.h"


boost::shared_mutex big_mutex;

Clients clients;


Client::Client(const string& name)
    : name(name), zombie(false)
{
}


Client::~Client()
{
    thread.interrupt();
    if (thread.joinable())
	thread.join();

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
	if (snap == snapshots.end())
	    throw IllegalSnapshotException();

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

    throw NoComparison();
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
	"      <arg name='configs' type='v' direction='out'/>\n"
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
	"      <arg name='snapshots' type='v' direction='out'/>\n"
	"    </method>\n"

	"    <method name='ListSnapshotsAtTime'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='begin' type='x' direction='in'/>\n"
	"      <arg name='end' type='x' direction='in'/>\n"
	"      <arg name='snapshots' type='v' direction='out'/>\n"
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

	"    <method name='MountSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='user-request' type='b' direction='in'/>\n"
	"    </method>\n"

	"    <method name='UmountSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='user-request' type='b' direction='in'/>\n"
	"    </method>\n"

	"    <method name='GetMountPoint'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
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
	"      <arg name='files' type='v' direction='out'/>\n"
	"    </method>\n"

	"  </interface>\n"
	"</node>\n";

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << introspect;

    conn.send(reply);
}


struct Permissions : public std::exception
{
    explicit Permissions() throw() {}
    virtual const char* what() const throw() { return "permissions"; }
};


void
Client::check_permission(DBus::Connection& conn, DBus::Message& msg) const
{
    unsigned long uid = conn.get_unix_userid(msg);
    if (uid == 0)
	return;

    throw Permissions();
}


void
Client::check_permission(DBus::Connection& conn, DBus::Message& msg,
			 const MetaSnapper& meta_snapper) const
{
    unsigned long uid = conn.get_unix_userid(msg);
    if (uid == 0)
	return;

    if (find(meta_snapper.uids.begin(), meta_snapper.uids.end(), uid) != meta_snapper.uids.end())
	return;

    throw Permissions();
}


struct Lock : public std::exception
{
    explicit Lock() throw() {}
    virtual const char* what() const throw() { return "locked"; }
};


void
Client::check_lock(DBus::Connection& conn, DBus::Message& msg, const string& config_name) const
{
    for (Clients::const_iterator it = clients.begin(); it != clients.end(); ++it)
    {
	if (it->zombie || &*it == this)
	    continue;

	if (it->has_lock(config_name))
	    throw Lock();
    }
}


struct ConfigInUse : public std::exception
{
    explicit ConfigInUse() throw() {}
    virtual const char* what() const throw() { return "config in use"; }
};


struct SnapshotInUse : public std::exception
{
    explicit SnapshotInUse() throw() {}
    virtual const char* what() const throw() { return "snapshot in use"; }
};


void
Client::check_config_in_use(const MetaSnapper& meta_snapper) const
{
    if (meta_snapper.use_count() != 0)
	throw ConfigInUse();
}


void
Client::check_snapshot_in_use(const MetaSnapper& meta_snapper, unsigned int number) const
{
    for (Clients::const_iterator it1 = clients.begin(); it1 != clients.end(); ++it1)
    {
	map<pair<string, unsigned int>, unsigned int>::const_iterator it2 =
	    it1->mounts.find(make_pair(meta_snapper.configName(), number));

	if (it2 != it1->mounts.end())
	    throw SnapshotInUse();
    }
}


void
Client::signal_config_created(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "ConfigCreated");

    DBus::Hoho hoho(msg);
    hoho << config_name;

    conn.send(msg);
}


void
Client::signal_config_modified(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "ConfigModified");

    DBus::Hoho hoho(msg);
    hoho << config_name;

    conn.send(msg);
}


void
Client::signal_config_deleted(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "ConfigDeleted");

    DBus::Hoho hoho(msg);
    hoho << config_name;

    conn.send(msg);
}


void
Client::signal_snapshot_created(DBus::Connection& conn, const string& config_name,
				unsigned int num)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "SnapshotCreated");

    DBus::Hoho hoho(msg);
    hoho << config_name << num;

    conn.send(msg);
}


void
Client::signal_snapshot_modified(DBus::Connection& conn, const string& config_name,
				 unsigned int num)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "SnapshotModified");

    DBus::Hoho hoho(msg);
    hoho << config_name << num;

    conn.send(msg);
}


void
Client::signal_snapshots_deleted(DBus::Connection& conn, const string& config_name,
				 const list<dbus_uint32_t>& nums)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "SnapshotsDeleted");

    DBus::Hoho hoho(msg);
    hoho << config_name << nums;

    conn.send(msg);
}


struct UnknownFile : public std::exception
{
    explicit UnknownFile() throw() {}
    virtual const char* what() const throw() { return "unknown config"; }
};


void
Client::list_configs(DBus::Connection& conn, DBus::Message& msg)
{
    y2deb("ListConfigs");

    boost::shared_lock<boost::shared_mutex> lock(big_mutex);

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho.open_array(DBus::TypeInfo<ConfigInfo>::signature);
    for (MetaSnappers::const_iterator it = meta_snappers.begin(); it != meta_snappers.end(); ++it)
	hoho << it->getConfigInfo();
    hoho.close_array();

    conn.send(reply);
}


void
Client::get_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Hihi hihi(msg);
    hihi >> config_name;

    y2deb("GetConfig config_name:" << config_name);

    boost::shared_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::const_iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << it->getConfigInfo();

    conn.send(reply);
}


void
Client::set_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Hihi hihi(msg);
    map<string, string> raw;
    hihi >> config_name >> raw;

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

    DBus::Hihi hihi(msg);
    hihi >> config_name >> subvolume >> fstype >> template_name;

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

    DBus::Hihi hihi(msg);
    hihi >> config_name;

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

    DBus::Hihi hihi(msg);
    hihi >> config_name;

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

    DBus::Hihi hihi(msg);
    hihi >> config_name;

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

    DBus::Hihi hihi(msg);
    hihi >> config_name;

    y2deb("ListSnapshots config_name:" << config_name);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << snapshots;

    conn.send(reply);
}


void
Client::list_snapshots_at_time(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    time_t begin, end;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> begin >> end;

    y2deb("ListSnapshotsAtTime config_name:" << config_name << " begin:" << begin <<
	  " end:" << end);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);

    hoho.open_array(DBus::TypeInfo<Snapshot>::signature);
    for (Snapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
    {
	if (it->getDate() >= begin && it->getDate() <= end)
	    hoho << *it;
    }
    hoho.close_array();

    conn.send(reply);
}


void
Client::get_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num;

    y2deb("GetSnapshot config_name:" << config_name << " num:" << num);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	throw IllegalSnapshotException();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << *snap;

    conn.send(reply);
}


void
Client::set_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;
    string description;
    string cleanup;
    map<string, string> userdata;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num >> description >> cleanup >> userdata;

    y2deb("SetSnapshot config_name:" << config_name << " num:" << num);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	throw IllegalSnapshotException();

    snapper->modifySnapshot(snap, description, cleanup, userdata);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);

    signal_snapshot_modified(conn, config_name, snap->getNum());
}


void
Client::create_single_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    string description;
    string cleanup;
    map<string, string> userdata;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> description >> cleanup >> userdata;

    y2deb("CreateSingleSnapshot config_name:" << config_name << " description:" << description <<
	  " cleanup:" << cleanup);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    Snapshots::iterator snap1 = snapper->createSingleSnapshot(conn.get_unix_userid(msg),
							      description, cleanup, userdata);

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << snap1->getNum();

    conn.send(reply);

    signal_snapshot_created(conn, config_name, snap1->getNum());
}


void
Client::create_pre_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    string description;
    string cleanup;
    map<string, string> userdata;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> description >> cleanup >> userdata;

    y2deb("CreatePreSnapshot config_name:" << config_name << " description:" << description <<
	  " cleanup:" << cleanup);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    Snapshots::iterator snap1 = snapper->createPreSnapshot(conn.get_unix_userid(msg), description,
							   cleanup, userdata);

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << snap1->getNum();

    conn.send(reply);

    signal_snapshot_created(conn, config_name, snap1->getNum());
}


void
Client::create_post_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    unsigned int pre_num;
    string description;
    string cleanup;
    map<string, string> userdata;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> pre_num >> description >> cleanup >> userdata;

    y2deb("CreatePostSnapshot config_name:" << config_name << " pre_num:" << pre_num <<
	  " description:" << description << " cleanup:" << cleanup);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap1 = snapshots.find(pre_num);

    Snapshots::iterator snap2 = snapper->createPostSnapshot(snap1, conn.get_unix_userid(msg),
							    description, cleanup, userdata);

    bool tmp;
    if (it->getConfigInfo().getValue("BACKGROUND_COMPARISON", tmp) && tmp)
	backgrounds.add_task(it, snap1, snap2);

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << snap2->getNum();

    conn.send(reply);

    signal_snapshot_created(conn, config_name, snap2->getNum());
}


void
Client::delete_snapshots(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    list<dbus_uint32_t> nums;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> nums;

    y2deb("DeleteSnapshots config_name:" << config_name << " nums:" << nums);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it1 = meta_snappers.find(config_name);

    check_permission(conn, msg, *it1);
    check_lock(conn, msg, config_name);
    check_config_in_use(*it1);

    Snapper* snapper = it1->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    for (list<unsigned int>::const_iterator it2 = nums.begin(); it2 != nums.end(); ++it2)
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
Client::mount_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;
    bool user_request;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num >> user_request;

    y2deb("MountSnapshot config_name:" << config_name << " num:" << num <<
	  " user_request:" << user_request);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	throw IllegalSnapshotException();

    snap->mountFilesystemSnapshot(user_request);

    if (!user_request)
	add_mount(config_name, num);

    string mount_point = snap->snapshotDir();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << mount_point;

    conn.send(reply);
}


void
Client::umount_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;
    bool user_request;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num >> user_request;

    y2deb("UmountSnapshot config_name:" << config_name << " num:" << num <<
	  " user_request:" << user_request);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	throw IllegalSnapshotException();

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

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num;

    y2deb("GetMountPoint config_name:" << config_name << " num:" << num);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();
    Snapshots& snapshots = snapper->getSnapshots();
    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
	throw IllegalSnapshotException();

    string mount_point = snap->snapshotDir();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << mount_point;

    conn.send(reply);
}


void
Client::create_comparison(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2;

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

    Comparison* comparison = new Comparison(snapper, snapshot1, snapshot2);

    lock.lock();

    comparisons.push_back(comparison);

    it->inc_use_count();

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::delete_comparison(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2;

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

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2;

    y2deb("GetFiles config_name:" << config_name << " num1:" << num1 << " num2:" << num2);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    list<Comparison*>::iterator it2 = find_comparison(it->getSnapper(), num1, num2);

    const Files& files = (*it2)->getFiles();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << files;

    conn.send(reply);
}


void
Client::debug(DBus::Connection& conn, DBus::Message& msg) const
{
    y2deb("Debug");

    boost::shared_lock<boost::shared_mutex> lock(big_mutex);

    check_permission(conn, msg);

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);

    hoho.open_array("s");

    hoho << "clients:";
    for (Clients::const_iterator it = clients.begin(); it != clients.end(); ++it)
    {
	std::ostringstream s;
	s << "    name:'" << it->name << "'";
	if (&*it == this)
	    s << ", myself";
	if (it->zombie)
	    s << ", zombie";
	if (!it->locks.empty())
	    s << ", locks " << it->locks.size();
	if (!it->comparisons.empty())
	    s << ", comparisons " << it->comparisons.size();
	hoho << s.str();
    }

    hoho << "backgrounds:";
    for (Backgrounds::const_iterator it = backgrounds.begin(); it != backgrounds.end(); ++it)
    {
	std::ostringstream s;
	s << "    name:'" << it->meta_snapper->configName() << "'";
	hoho << s.str();
    }

    hoho << "meta-snappers:";
    for (MetaSnappers::const_iterator it = meta_snappers.begin(); it != meta_snappers.end(); ++it)
    {
	std::ostringstream s;
	s << "    name:'" << it->configName() << "'";
	if (it->is_loaded())
	{
	    s << ", loaded";
	    if (it->use_count() == 0)
		s << ", unused for " << it->unused_for() << "s";
	    else
		s << ", use count " << it->use_count();
	}
	hoho << s.str();
    }

    hoho.close_array();

    conn.send(reply);
}


void
Client::dispatch(DBus::Connection& conn, DBus::Message& msg)
{
    try
    {
	if (msg.is_method_call(INTERFACE, "ListConfigs"))
	    list_configs(conn, msg);
	else if (msg.is_method_call(INTERFACE, "CreateConfig"))
	    create_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetConfig"))
	    get_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "SetConfig"))
	    set_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "DeleteConfig"))
	    delete_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "LockConfig"))
	    lock_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "UnlockConfig"))
	    unlock_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "ListSnapshots"))
	    list_snapshots(conn, msg);
	else if (msg.is_method_call(INTERFACE, "ListSnapshotsAtTime"))
	    list_snapshots_at_time(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetSnapshot"))
	    get_snapshot(conn, msg);
	else if (msg.is_method_call(INTERFACE, "SetSnapshot"))
	    set_snapshot(conn, msg);
	else if (msg.is_method_call(INTERFACE, "CreateSingleSnapshot"))
	    create_single_snapshot(conn, msg);
	else if (msg.is_method_call(INTERFACE, "CreatePreSnapshot"))
	    create_pre_snapshot(conn, msg);
	else if (msg.is_method_call(INTERFACE, "CreatePostSnapshot"))
	    create_post_snapshot(conn, msg);
	else if (msg.is_method_call(INTERFACE, "DeleteSnapshots"))
	    delete_snapshots(conn, msg);
	else if (msg.is_method_call(INTERFACE, "MountSnapshot"))
	    mount_snapshot(conn, msg);
	else if (msg.is_method_call(INTERFACE, "UmountSnapshot"))
	    umount_snapshot(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetMountPoint"))
	    get_mount_point(conn, msg);
	else if (msg.is_method_call(INTERFACE, "CreateComparison"))
	    create_comparison(conn, msg);
	else if (msg.is_method_call(INTERFACE, "DeleteComparison"))
	    delete_comparison(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetFiles"))
	    get_files(conn, msg);
	else if (msg.is_method_call(INTERFACE, "Debug"))
	    debug(conn, msg);
	else
	{
	    DBus::MessageError reply(msg, "error.unknown_method", DBUS_ERROR_FAILED);
	    conn.send(reply);
	}
    }
    catch (const boost::thread_interrupted&)
    {
	throw;
    }
    catch (const DBus::MarshallingException& e)
    {
	DBus::MessageError reply(msg, "error.dbus.marshalling", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const DBus::FatalException& e)
    {
	DBus::MessageError reply(msg, "error.dbus.fatal", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const UnknownConfig& e)
    {
	DBus::MessageError reply(msg, "error.unknown_config", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const CreateConfigFailedException& e)
    {
	DBus::MessageError reply(msg, "error.create_config_failed", e.what());
	conn.send(reply);
    }
    catch (const DeleteConfigFailedException& e)
    {
	DBus::MessageError reply(msg, "error.delete_config_failed", e.what());
	conn.send(reply);
    }
    catch (const Permissions& e)
    {
	DBus::MessageError reply(msg, "error.no_permissions", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const Lock& e)
    {
	DBus::MessageError reply(msg, "error.config_locked", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const ConfigInUse& e)
    {
	DBus::MessageError reply(msg, "error.config_in_use", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const SnapshotInUse& e)
    {
	DBus::MessageError reply(msg, "error.snapshot_in_use", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const NoComparison& e)
    {
	DBus::MessageError reply(msg, "error.no_comparisons", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const IllegalSnapshotException& e)
    {
	DBus::MessageError reply(msg, "error.illegal_snapshot", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const CreateSnapshotFailedException& e)
    {
	DBus::MessageError reply(msg, "error.create_snapshot_failed", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const DeleteSnapshotFailedException& e)
    {
	DBus::MessageError reply(msg, "error.delete_snapshot_failed", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const UnknownFile& e)
    {
	DBus::MessageError reply(msg, "error.unknown_file", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const InvalidConfigdataException& e)
    {
	DBus::MessageError reply(msg, "error.invalid_configdata", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const InvalidUserdataException& e)
    {
	DBus::MessageError reply(msg, "error.invalid_userdata", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const IOErrorException& e)
    {
	DBus::MessageError reply(msg, "error.io_error", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const IsSnapshotMountedFailedException& e)
    {
	DBus::MessageError reply(msg, "error.is_snapshot_mounted", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const MountSnapshotFailedException& e)
    {
	DBus::MessageError reply(msg, "error.mount_snapshot", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (const UmountSnapshotFailedException& e)
    {
	DBus::MessageError reply(msg, "error.umount_snapshot", DBUS_ERROR_FAILED);
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
Client::add_task(DBus::Connection& conn, DBus::Message& msg)
{
    if (thread.get_id() == boost::thread::id())
	thread = boost::thread(boost::bind(&Client::worker, this));

    boost::unique_lock<boost::mutex> lock(mutex);
    tasks.push(Task(conn, msg));
    lock.unlock();

    condition.notify_one();
}


void
Client::worker()
{
    try
    {
	while (true)
	{
	    boost::unique_lock<boost::mutex> lock(mutex);
	    while (tasks.empty())
		condition.wait(lock);
	    Task task = tasks.front();
	    tasks.pop();
	    lock.unlock();

	    dispatch(task.conn, task.msg);
	}
    }
    catch (const boost::thread_interrupted&)
    {
	y2deb("worker interrupted");
    }
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
Clients::add(const string& name)
{
    assert(find(name) == entries.end());

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 4)
    entries.emplace_back(name);
#else
    entries.push_back(name);
#endif

    return --entries.end();
}


void
Clients::remove_zombies()
{
    for (iterator it = begin(); it != end();)
    {
	if (it->zombie && it->thread.timed_join(boost::posix_time::seconds(0)))
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
