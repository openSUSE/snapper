/*
 * Copyright (c) 2012 Novell, Inc.
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


#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#include <iostream>
#include <string>
#include <boost/algorithm/string.hpp>

#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/Comparison.h>
#include <snapper/Log.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/AsciiFile.h>

#include "dbus/DBusMessage.h"
#include "dbus/DBusConnection.h"
#include "dbus/DBusMainLoop.h"

#include "MetaSnapper.h"
#include "Client.h"
#include "Types.h"


using namespace std;
using namespace snapper;


#define SERVICE "org.opensuse.Snapper"
#define PATH "/org/opensuse/Snapper"
#define INTERFACE "org.opensuse.Snapper"


Clients clients;


boost::shared_mutex big_mutex;


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

	"    <signal name='ConfigDeleted'>\n"
	"      <arg name='config-name' type='s'/>\n"
	"    </signal>\n"

	"    <signal name='SnapshotCreated'>\n"
	"      <arg name='config-name' type='s'/>\n"
	"      <arg name='number' type='u'/>\n"
	"    </signal>\n"

	"    <signal name='SnapshotDeleted'>\n"
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

	"    <method name='GetSnapshot'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number' type='u' direction='in'/>\n"
	"      <arg name='type' type='(uquussa{ss})' direction='out'/>\n"
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
	"    </method>\n"

	"    <method name='UmountSnapshot'>\n"
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

	"    <method name='GetDiff'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
	"      <arg name='filename' type='s' direction='in'/>\n"
	"      <arg name='options' type='s' direction='in'/>\n"
	"      <arg name='diff' type='v' direction='out'/>\n"
	"    </method>\n"

	"    <method name='SetUndo'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
	"      <arg name='files' type='a(sb)' direction='in'/>\n"
	"    </method>\n"

	"    <method name='SetUndoAll'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
	"      <arg name='undo' type='b' direction='in'/>\n"
	"    </method>\n"

	"    <method name='GetUndoSteps'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
	"      <arg name='undo-steps' type='a(sq)' direction='out'/>\n"
	"    </method>\n"

	"    <method name='DoUndoStep'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
	"      <arg name='undo-step' type='(sq)' direction='in'/>\n"
	"    </method>\n"

	"  </interface>\n"
	"</node>\n";

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << introspect;

    conn.send(reply);
}


struct UnknownConfig : public std::exception
{
    explicit UnknownConfig() throw() {}
    virtual const char* what() const throw() { return "unknown config"; }
};


struct Permissions : public std::exception
{
    explicit Permissions() throw() {}
    virtual const char* what() const throw() { return "permissions"; }
};


void
check_permission(DBus::Connection& conn, DBus::Message& msg)
{
    unsigned long uid = conn.get_unix_userid(msg);
    if (uid == 0)
	return;

    throw Permissions();
}


void
check_permission(DBus::Connection& conn, DBus::Message& msg, const MetaSnapper& meta_snapper)
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
check_lock(DBus::Connection& conn, DBus::Message& msg, const string& config_name)
{
    for (Clients::const_iterator it = clients.begin(); it != clients.end(); ++it)
    {
	if (it->name == msg.get_sender())
	    continue;

	if (it->has_lock(config_name))
	    throw Lock();
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
Client::signal_snapshots_deleted(DBus::Connection& conn, const string& config_name,
				   list<dbus_uint32_t> nums)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "SnapshotsDeleted");

    DBus::Hoho hoho(msg);
    hoho << config_name << nums;

    conn.send(msg);
}


void
Client::list_configs(DBus::Connection& conn, DBus::Message& msg)
{
    y2deb("ListConfigs");

    boost::shared_lock<boost::shared_mutex> lock(big_mutex);

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho.open_array(DBus::TypeInfo<ConfigInfo>::signature);
    for (MetaSnappers::const_iterator it = meta_snappers.begin(); it != meta_snappers.end(); ++it)
	hoho << it->config_info;
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
    hoho << it->config_info;

    conn.send(reply);
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

    Snapper::createConfig(config_name, subvolume, fstype, template_name);

    // TODO update meta_snappers

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

    check_permission(conn, msg);

    Snapper::deleteConfig(config_name);

    // TODO update meta_snappers

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

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << snapper->getSnapshots();

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

    snap->setDescription(description);
    snap->setCleanup(cleanup);
    snap->setUserdata(userdata);
    snap->flushInfo();

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
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

    Snapshots::iterator snap1 = snapper->createSingleSnapshot(description);
    snap1->setCleanup(cleanup);
    snap1->setUserdata(userdata);
    snap1->flushInfo();

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

    Snapshots::iterator snap1 = snapper->createPreSnapshot(description);
    snap1->setCleanup(cleanup);
    snap1->setUserdata(userdata);
    snap1->flushInfo();

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

    Snapshots::iterator snap2 = snapper->createPostSnapshot(description, snap1);
    snap2->setCleanup(cleanup);
    snap2->setUserdata(userdata);
    snap2->flushInfo();

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

    check_lock(conn, msg, config_name);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    Snapshots& snapshots = snapper->getSnapshots();

    for (list<unsigned int>::const_iterator it = nums.begin(); it != nums.end(); ++it)
    {
	Snapshots::iterator snap = snapshots.find(*it);

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

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num;

    y2deb("MountSnapshot config_name:" << config_name << " num:" << num);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);

    snap->mountFilesystemSnapshot();

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::umount_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num;

    y2deb("UmountSnapshot config_name:" << config_name << " num:" << num);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Snapper* snapper = it->getSnapper();

    Snapshots& snapshots = snapper->getSnapshots();

    Snapshots::iterator snap = snapshots.find(num);

    snap->umountFilesystemSnapshot();

    DBus::MessageMethodReturn reply(msg);

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

    lock.unlock();

    Comparison* comparison = new Comparison(snapper, snapshot1, snapshot2);

    lock.lock();

    comparisons.push_back(comparison);

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

    Comparison* comparison = find_comparison(it->getSnapper(), num1, num2);

    const Files& files = comparison->getFiles();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << files;

    conn.send(reply);
}


void
Client::get_diff(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;
    string filename;
    string options;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2 >> filename >> options;

    y2deb("GetDiff config_name:" << config_name << " num1:" << num1 << " num2:" << num2 <<
	  " filename:" << filename << " options:" << options);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Comparison* comparison = find_comparison(it->getSnapper(), num1, num2);

    Files& files = comparison->getFiles();

    Files::iterator it3 = files.find(filename);
    assert(it3 != files.end());

    lock.unlock();

    vector<string> d = it3->getDiff(options);

    lock.lock();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << d;

    conn.send(reply);
}


void
Client::set_undo(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;
    list<Undo> undos;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2 >> undos;

    y2deb("SetUndo config_name:" << config_name << " num1:" << num1 << " num2:" << num2);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Comparison* comparison = find_comparison(it->getSnapper(), num1, num2);

    Files& files = comparison->getFiles();

    for (list<Undo>::const_iterator it2 = undos.begin(); it2 != undos.end(); ++it2)
    {
	Files::iterator it3 = files.find(it2->filename);
	if (it3 == files.end())
	    throw;

	it3->setUndo(it2->undo);
    }

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::set_undo_all(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;
    bool undo;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2 >> undo;

    y2deb("SetUndoAll config_name:" << config_name << " num1:" << num1 << " num2:" << num2);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Comparison* comparison = find_comparison(it->getSnapper(), num1, num2);

    Files& files = comparison->getFiles();

    for (Files::iterator it2 = files.begin(); it2 != files.end(); ++it2)
    {
	it2->setUndo(undo);
    }

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
Client::get_undo_steps(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2;

    y2deb("GetUndoSteps config_name:" << config_name << " num1:" << num1 << " num2:" << num2);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Comparison* comparison = find_comparison(it->getSnapper(), num1, num2);

    vector<UndoStep> undo_steps = comparison->getUndoSteps();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << undo_steps;

    conn.send(reply);
}


void
Client::do_undo_step(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;
    UndoStep undo_step("", MODIFY);

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2 >> undo_step;

    y2deb("DoUndoStep config_name:" << config_name << " num1:" << num1 << " num2:" << num2);

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    MetaSnappers::iterator it = meta_snappers.find(config_name);

    check_permission(conn, msg, *it);

    Comparison* comparison = find_comparison(it->getSnapper(), num1, num2);

    lock.unlock();

    bool ret = comparison->doUndoStep(undo_step);

    lock.lock();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << ret;

    conn.send(reply);
}


void
Client::debug(DBus::Connection& conn, DBus::Message& msg)
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
	    s << " myself";
	if (it->zombie)
	    s << " zombie";
	if (!it->locks.empty())
	    s << " locks:'" << boost::join(it->locks, ",") << "'";
	if (!it->comparisons.empty())
	    s << " comparisons:" << it->comparisons.size();
	hoho << s.str();
    }

    hoho << "meta-snappers:";
    for (MetaSnappers::const_iterator it = meta_snappers.begin(); it != meta_snappers.end(); ++it)
    {
	std::ostringstream s;
	s << "    name:'" << it->configName() << "'";
	if (it->snapper_loaded())
	    s << " (loaded)";
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
	else if (msg.is_method_call(INTERFACE, "DeleteConfig"))
	    delete_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "LockConfig"))
	    lock_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "UnlockConfig"))
	    unlock_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "ListSnapshots"))
	    list_snapshots(conn, msg);
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
	else if (msg.is_method_call(INTERFACE, "CreateComparison"))
	    create_comparison(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetFiles"))
	    get_files(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetDiff"))
	    get_diff(conn, msg);
	else if (msg.is_method_call(INTERFACE, "SetUndo"))
	    set_undo(conn, msg);
	else if (msg.is_method_call(INTERFACE, "SetUndoAll"))
	    set_undo_all(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetUndoSteps"))
	    get_undo_steps(conn, msg);
	else if (msg.is_method_call(INTERFACE, "DoUndoStep"))
	    do_undo_step(conn, msg);
	else if (msg.is_method_call(INTERFACE, "Debug"))
	    debug(conn, msg);
	else
	{
	    DBus::MessageError reply(msg, "error.unknown_method", DBUS_ERROR_FAILED);
	    conn.send(reply);
	}
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
    catch (const InvalidUserdataException& e)
    {
	DBus::MessageError reply(msg, "error.invalid_userdata", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
    catch (...)
    {
	DBus::MessageError reply(msg, "error.something", DBUS_ERROR_FAILED);
	conn.send(reply);
    }
}


class MyMainLoop : public DBus::MainLoop
{
public:

    MyMainLoop(DBusBusType type);
    ~MyMainLoop();

    void method_call(DBus::Message& message);
    void signal(DBus::Message& message);
    void client_connected(const string& name);
    void client_disconnected(const string& name);
    void periodic();
    int periodic_timeout();

};


MyMainLoop::MyMainLoop(DBusBusType type)
    : MainLoop(type)
{
}


MyMainLoop::~MyMainLoop()
{
}


void
MyMainLoop::method_call(DBus::Message& msg)
{
    y2deb("method call sender:'" << msg.get_sender() << "' path:'" <<
	  msg.get_path() << "' interface:'" << msg.get_interface() <<
	  "' member:'" << msg.get_member() << "'");

    reset_idle_count();

    if (msg.is_method_call(DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
    {
	Client::introspect(*this, msg);
    }
    else
    {
	boost::unique_lock<boost::shared_mutex> lock(big_mutex);

	Clients::iterator client = clients.find(msg.get_sender());
	if (client == clients.end())
	{
	    y2deb("client connected invisible '" << msg.get_sender() << "'");
	    client = clients.add(msg.get_sender());
	    set_idle_timeout(-1);
	}

	client->add_task(*this, msg);
    }
}


void
MyMainLoop::signal(DBus::Message& msg)
{
    y2deb("signal sender:'" << msg.get_sender() << "' path:'" <<
	  msg.get_path() << "' interface:'" << msg.get_interface() <<
	  "' member:'" << msg.get_member() << "'");
}


void
MyMainLoop::client_connected(const string& name)
{
    y2deb("client connected '" << name << "'");

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    clients.add(name);

    reset_idle_count();
    set_idle_timeout(-1);
}


void
MyMainLoop::client_disconnected(const string& name)
{
    y2deb("client disconnected '" << name << "'");

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    Clients::iterator client = clients.find(name);
    if (client != clients.end())
    {
	client->zombie = true;
	client->thread.interrupt();
    }
    reset_idle_count();
}


void
MyMainLoop::periodic()
{
    y2deb("periodic");

    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    clients.remove_zombies();

    if (clients.empty())
    {
	y2deb("no clients left");
	set_idle_timeout(30);
    }
}


int
MyMainLoop::periodic_timeout()
{
    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    if (clients.has_zombies())
	return 1000;

    return -1;
}


void
log_do(LogLevel level, const string& component, const char* file, const int line, const char* func,
       const string& text)
{
    cout << /* boost::this_thread::get_id() << " " << */ text << endl;
}


bool
log_query(LogLevel level, const string& component)
{
    return true;
    // return level != DEBUG;
}


int
main(int argc, char** argv)
{
#if 0
    initDefaultLogger();
#else
    setLogDo(&log_do);
    setLogQuery(&log_query);
#endif

    y2mil("Loading snapper configs");

    meta_snappers.init();

    dbus_threads_init_default();

    MyMainLoop mainloop(DBUS_BUS_SYSTEM);

    mainloop.set_idle_timeout(30);

    y2mil("Requesting DBus name");

    mainloop.request_name("org.opensuse.Snapper", DBUS_NAME_FLAG_REPLACE_EXISTING);

    y2mil("Listening for method calls and signals");

    mainloop.run();

    y2mil("Exiting");

    return 0;
}
