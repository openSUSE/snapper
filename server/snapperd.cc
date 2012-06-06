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

#include <string>
#include <boost/algorithm/string.hpp>

#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/Comparison.h>
#include <snapper/Log.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/AsciiFile.h>

#include "dbus/DBusConnection.h"
#include "dbus/DBusMessage.h"

#include "MetaSnapper.h"
#include "Client.h"
#include "Job.h"
#include "Types.h"


using namespace std;
using namespace snapper;


#define SERVICE "org.opensuse.snapper"
#define PATH "/org/opensuse/snapper"
#define INTERFACE "org.opensuse.snapper"


Clients clients;

Jobs jobs;


void
reply_to_introspect(DBus::Connection& conn, DBus::Message& msg)
{
    const char* introspect =
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE "\n"
	"<node name='/org/opensuse/snapper'>\n"
	"  <interface name='" DBUS_INTERFACE_INTROSPECTABLE "'>\n"
	"    <method name='Introspect'>\n"
	"      <arg name='xml_data' type='s' direction='out'/>\n"
	"    </method>\n"
	"  </interface>\n"
	"  <interface name='" INTERFACE "'>\n"

	"    <method name='ListConfigs'>\n"
	"      <arg name='configs' type='v' direction='out'/>\n"
	"    </method>\n"

	"    <method name='CreateConfig'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='subvolume' type='s' direction='in'/>\n"
	"      <arg name='fstype' type='s' direction='in'/>\n"
	"      <arg name='template-name' type='s' direction='in'/>\n"
	"    </method>\n"

	"    <method name='GetConfig'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='data' type='a{ss}' direction='out'/>\n"
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

	"    <method name='GetUndoStatistic'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
	"      <arg name='number-create' type='u' direction='out'/>\n"
	"      <arg name='number-modify' type='u' direction='out'/>\n"
	"      <arg name='number-delete' type='u' direction='out'/>\n"
	"    </method>\n"

	"    <method name='UndoChanges'>\n"
	"      <arg name='config-name' type='s' direction='in'/>\n"
	"      <arg name='number1' type='u' direction='in'/>\n"
	"      <arg name='number2' type='u' direction='in'/>\n"
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
check_permission(DBus::Connection& conn, DBus::Message& msg, const string& config_name)
{
    list<ConfigInfo> config_infos = Snapper::getConfigs();

    for (list<ConfigInfo>::const_iterator it = config_infos.begin();
	 it != config_infos.end(); ++it)
    {
	if (it->config_name == config_name)
	{
	    unsigned long uid = conn.get_unix_userid(msg);
	    if (uid == 0)
		return;

	    string username = conn.get_unix_username(msg);
	    if (find(it->users.begin(), it->users.end(), username) != it->users.end())
		return;

	    throw Permissions();
	}
    }

    throw UnknownConfig();
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
send_signal_config_created(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "ConfigCreated");

    DBus::Hoho hoho(msg);
    hoho << config_name;

    conn.send(msg);
}


void
send_signal_config_deleted(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "ConfigDeleted");

    DBus::Hoho hoho(msg);
    hoho << config_name;

    conn.send(msg);
}


void
send_signal_snapshot_created(DBus::Connection& conn, const string& config_name,
			     unsigned int num)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "SnapshotCreated");

    DBus::Hoho hoho(msg);
    hoho << config_name << num;

    conn.send(msg);
}


void
send_signal_snapshots_deleted(DBus::Connection& conn, const string& config_name,
			      list<dbus_uint32_t> nums)
{
    DBus::MessageSignal msg(PATH, INTERFACE, "SnapshotsDeleted");

    DBus::Hoho hoho(msg);
    hoho << config_name << nums;

    conn.send(msg);
}


void
reply_to_command_list_configs(DBus::Connection& conn, DBus::Message& msg)
{
    y2mil("ListConfigs");

    list<ConfigInfo> config_infos = Snapper::getConfigs();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << config_infos;

    conn.send(reply);
}


void
reply_to_command_create_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    string subvolume;
    string fstype;
    string template_name;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> subvolume >> fstype >> template_name;

    y2mil("CreateConfig config_name:" << config_name << " subvolume:" << subvolume <<
	  " fstype:" << fstype << " template_name:" << template_name);

    check_permission(conn, msg);

    Snapper::createConfig(config_name, subvolume, fstype, template_name);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);

    send_signal_config_created(conn, config_name);
}


void
reply_to_command_get_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Hihi hihi(msg);
    hihi >> config_name;

    y2mil("GetConfig config_name:" << config_name);

    check_permission(conn, msg, config_name);

    Snapper* snapper = getSnapper(config_name);

    map<string, string> tmp = snapper->getSysconfigFile()->getAllValues();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << tmp;

    conn.send(reply);
}


void
reply_to_command_delete_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Hihi hihi(msg);
    hihi >> config_name;

    y2mil("DeleteConfig config_name:" << config_name);

    check_permission(conn, msg);

    Snapper::deleteConfig(config_name);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);

    send_signal_config_deleted(conn, config_name);
}


void
reply_to_command_lock_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Hihi hihi(msg);
    hihi >> config_name;

    y2mil("LockConfig config_name:" << config_name);

    check_permission(conn, msg, config_name);

    string sender = msg.get_sender();

    Clients::iterator it = clients.find(sender);
    it->add_lock(config_name);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
reply_to_command_unlock_config(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Hihi hihi(msg);
    hihi >> config_name;

    y2mil("UnlockConfig config_name:" << config_name);

    check_permission(conn, msg, config_name);

    string sender = msg.get_sender();

    Clients::iterator it = clients.find(sender);
    it->remove_lock(config_name);

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
reply_to_command_list_snapshots(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;

    DBus::Hihi hihi(msg);
    hihi >> config_name;

    y2mil("ListSnapshots config_name:" << config_name);

    check_permission(conn, msg, config_name);

    Snapper* snapper = getSnapper(config_name);

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << snapper->getSnapshots();

    conn.send(reply);
}


void
reply_to_command_create_single_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    string description;
    string cleanup;
    map<string, string> userdata;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> description >> cleanup >> userdata;

    y2mil("CreateSingleSnapshot config_name:" << config_name << " description:" << description <<
	  " cleanup:" << cleanup);

    check_permission(conn, msg, config_name);

    Snapper* snapper = getSnapper(config_name);

    Snapshots::iterator snap1 = snapper->createSingleSnapshot(description);
    snap1->setCleanup(cleanup);
    snap1->setUserdata(userdata);
    snap1->flushInfo();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << snap1->getNum();

    conn.send(reply);

    send_signal_snapshot_created(conn, config_name, snap1->getNum());
}


void
reply_to_command_create_pre_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    string description;
    string cleanup;
    map<string, string> userdata;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> description >> cleanup >> userdata;

    y2mil("CreatePreSnapshot config_name:" << config_name << " description:" << description <<
	  " cleanup:" << cleanup);

    check_permission(conn, msg, config_name);

    Snapper* snapper = getSnapper(config_name);

    Snapshots::iterator snap1 = snapper->createPreSnapshot(description);
    snap1->setCleanup(cleanup);
    snap1->setUserdata(userdata);
    snap1->flushInfo();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << snap1->getNum();

    conn.send(reply);

    send_signal_snapshot_created(conn, config_name, snap1->getNum());
}


void
reply_to_command_create_post_snapshot(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    unsigned int pre_num;
    string description;
    string cleanup;
    map<string, string> userdata;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> pre_num >> description >> cleanup >> userdata;

    y2mil("CreatePostSnapshot config_name:" << config_name << " pre_num:" << pre_num <<
	  " description:" << description << " cleanup:" << cleanup);

    check_permission(conn, msg, config_name);

    Snapper* snapper = getSnapper(config_name);

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

    send_signal_snapshot_created(conn, config_name, snap2->getNum());
}


void
reply_to_command_delete_snapshots(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    list<dbus_uint32_t> nums;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> nums;

    y2mil("DeleteSnapshots config_name:" << config_name << " nums:" << nums);

    check_permission(conn, msg, config_name);
    check_lock(conn, msg, config_name);

    Snapper* snapper = getSnapper(config_name);

    Snapshots& snapshots = snapper->getSnapshots();

    for (list<unsigned int>::const_iterator it = nums.begin(); it != nums.end(); ++it)
    {
	Snapshots::iterator snap = snapshots.find(*it);

	snapper->deleteSnapshot(snap);
    }

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);

    send_signal_snapshots_deleted(conn, config_name, nums);
}


struct Comparing : public Job
{
    DBus::Connection* conn;
    DBus::MessageMethodReturn* reply;

    Comparison* comparison;

    void done();

protected:

    virtual void operator()();

};


void
reply_to_command_create_comparison(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2;

    y2mil("CreateComparison config_name:" << config_name << " num1:" << num1 <<
	  " num2:" << num2);

    check_permission(conn, msg, config_name);

    Snapper* snapper = getSnapper(config_name);
    Snapshots& snapshots = snapper->getSnapshots();
    Snapshots::const_iterator snapshot1 = snapshots.find(num1);
    Snapshots::const_iterator snapshot2 = snapshots.find(num2);

    Comparison* comparison = new Comparison(snapper, snapshot1, snapshot2, true);

    Clients::iterator it = clients.find(msg.get_sender());
    assert(it != clients.end());
    it->comparisons.push_back(comparison);

    Comparing* job = new Comparing;
    job->conn = &conn;
    job->reply = new DBus::MessageMethodReturn(msg);
    job->comparison = comparison;

    jobs.add(job);
}


void
Comparing::operator()()
{
    boost::this_thread::sleep(seconds(2));

    comparison->initialize();

    boost::this_thread::sleep(seconds(2));
}


void
Comparing::done()
{
    DBus::Hoho hoho(*reply);
    conn->send(*reply);

    delete reply;
}


void
reply_to_command_get_files(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2;

    y2mil("GetFiles config_name:" << config_name << " num1:" << num1 << " num2:" <<
	  num2);

    check_permission(conn, msg, config_name);

    string sender = msg.get_sender();

    Clients::iterator it = clients.find(sender);
    assert(it != clients.end());

    Comparison* comparison = it->find_comparison(config_name, num1, num2);

    const Files& files = comparison->getFiles();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << files;

    conn.send(reply);
}


void
reply_to_command_get_diff(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;
    string filename;
    string options;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2 >> filename >> options;

    y2mil("GetDiff config_name:" << config_name << " num1:" << num1 << " num2:" <<
	  num2 << " filename:" << filename << " options:" << options);

    check_permission(conn, msg, config_name);

    string sender = msg.get_sender();

    Clients::iterator it = clients.find(sender);
    assert(it != clients.end());

    Comparison* comparison = it->find_comparison(config_name, num1, num2);

    Files& files = comparison->getFiles();

    Files::iterator it3 = files.find(filename);
    assert(it3 != files.end());

    vector<string> d = it3->getDiff(options);

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << d;

    conn.send(reply);
}


void
reply_to_command_set_undo(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;
    list<Undo> undos;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2 >> undos;

    y2mil("SetUndo config_name:" << config_name << " num1:" << num1 << " num2:" <<
	  num2);

    check_permission(conn, msg, config_name);

    string sender = msg.get_sender();

    Clients::iterator it = clients.find(sender);
    assert(it != clients.end());

    Comparison* comparison = it->find_comparison(config_name, num1, num2);

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
reply_to_command_set_undo_all(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;
    bool undo;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2 >> undo;

    y2mil("SetUndoAll config_name:" << config_name << " num1:" << num1 << " num2:" <<
	  num2);

    check_permission(conn, msg, config_name);

    string sender = msg.get_sender();

    Clients::iterator it = clients.find(sender);
    assert(it != clients.end());

    Comparison* comparison = it->find_comparison(config_name, num1, num2);

    Files& files = comparison->getFiles();

    for (Files::iterator it2 = files.begin(); it2 != files.end(); ++it2)
    {
	it2->setUndo(undo);
    }

    DBus::MessageMethodReturn reply(msg);

    conn.send(reply);
}


void
reply_to_command_get_undo_statistics(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2;

    y2mil("GetUndoStatistic config_name:" << config_name << " num1:" << num1 << " num2:" <<
	  num2);

    check_permission(conn, msg, config_name);

    string sender = msg.get_sender();

    Clients::iterator it = clients.find(sender);
    assert(it != clients.end());

    Comparison* comparison = it->find_comparison(config_name, num1, num2);

    UndoStatistic statistic = comparison->getUndoStatistic();

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);
    hoho << statistic.numCreate << statistic.numModify << statistic.numDelete;

    conn.send(reply);
}


struct Undoing : public Job
{
    DBus::Connection* conn;
    DBus::MessageMethodReturn* reply;

    Comparison* comparison;

    void done();

protected:

    virtual void operator()();

};


void
reply_to_command_undo_changes(DBus::Connection& conn, DBus::Message& msg)
{
    string config_name;
    dbus_uint32_t num1, num2;

    DBus::Hihi hihi(msg);
    hihi >> config_name >> num1 >> num2;

    y2mil("UndoChanges config_name:" << config_name << " num1:" << num1 << " num2:" <<
	  num2);

    check_permission(conn, msg, config_name);

    string sender = msg.get_sender();

    Clients::iterator it = clients.find(sender);
    assert(it != clients.end());

    Comparison* comparison = it->find_comparison(config_name, num1, num2);

    Undoing* job = new Undoing;
    job->conn = &conn;
    job->reply = new DBus::MessageMethodReturn(msg);
    job->comparison = comparison;

    jobs.add(job);
}


void
Undoing::operator()()
{
    boost::this_thread::sleep(seconds(2));

    comparison->doUndo();

    boost::this_thread::sleep(seconds(2));
}


void
Undoing::done()
{
    DBus::Hoho hoho(*reply);
    conn->send(*reply);

    delete reply;
}


void
reply_to_command_debug(DBus::Connection& conn, DBus::Message& msg)
{
    // check_permission(conn, msg);

    DBus::MessageMethodReturn reply(msg);

    DBus::Hoho hoho(reply);

    hoho.open_array("s");

    hoho << "clients:";
    for (list<Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
	std::ostringstream s;
	s << "    name:'" << it->name << "'";
	if (it->name == msg.get_sender())
	    s << " myself";
	s << " username:'" << conn.get_unix_username(msg) << "'";
	if (!it->locks.empty())
	    s << " locks:'" << boost::join(it->locks, ",") << "'";
	if (!it->comparisons.empty())
	    s << " comparisons:" << it->comparisons.size();
	hoho << s.str();
    }

    hoho << "snappers:";
    for (list<Snapper*>::iterator it = snappers.begin(); it != snappers.end(); ++it)
    {
	std::ostringstream s;
	s << "    name:'" << (*it)->configName() << "'";
	hoho << s.str();
    }

    if (!jobs.empty())
    {
	std::ostringstream s;
	s << "running jobs:" << jobs.size();
	hoho << s.str();
    }

    hoho.close_array();

    conn.send(reply);
}


void
client_connected(const string& name)
{
    clients.add(name);
}


void
client_disconnected(const string& name)
{
    clients.remove(name);
}


void
dispatch(DBus::Connection& conn, DBus::Message& msg)
{
    try
    {
	if (msg.is_method_call(INTERFACE, "ListConfigs"))
	    reply_to_command_list_configs(conn, msg);
	else if (msg.is_method_call(INTERFACE, "CreateConfig"))
	    reply_to_command_create_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetConfig"))
	    reply_to_command_get_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "DeleteConfig"))
	    reply_to_command_delete_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "LockConfig"))
	    reply_to_command_lock_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "UnlockConfig"))
	    reply_to_command_unlock_config(conn, msg);
	else if (msg.is_method_call(INTERFACE, "ListSnapshots"))
	    reply_to_command_list_snapshots(conn, msg);
	else if (msg.is_method_call(INTERFACE, "CreateSingleSnapshot"))
	    reply_to_command_create_single_snapshot(conn, msg);
	else if (msg.is_method_call(INTERFACE, "CreatePreSnapshot"))
	    reply_to_command_create_pre_snapshot(conn, msg);
	else if (msg.is_method_call(INTERFACE, "CreatePostSnapshot"))
	    reply_to_command_create_post_snapshot(conn, msg);
	else if (msg.is_method_call(INTERFACE, "DeleteSnapshots"))
	    reply_to_command_delete_snapshots(conn, msg);
	else if (msg.is_method_call(INTERFACE, "CreateComparison"))
	    reply_to_command_create_comparison(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetFiles"))
	    reply_to_command_get_files(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetDiff"))
	    reply_to_command_get_diff(conn, msg);
	else if (msg.is_method_call(INTERFACE, "SetUndo"))
	    reply_to_command_set_undo(conn, msg);
	else if (msg.is_method_call(INTERFACE, "SetUndoAll"))
	    reply_to_command_set_undo_all(conn, msg);
	else if (msg.is_method_call(INTERFACE, "GetUndoStatistic"))
	    reply_to_command_get_undo_statistics(conn, msg);
	else if (msg.is_method_call(INTERFACE, "UndoChanges"))
	    reply_to_command_undo_changes(conn, msg);
	else if (msg.is_method_call(INTERFACE, "Debug"))
	    reply_to_command_debug(conn, msg);
	else
	{
	    DBus::MessageError reply(msg, "error.unknown", DBUS_ERROR_FAILED);
	    conn.send(reply);
	}
    }
    catch (const DBus::MarshallingException& e)
    {
	DBus::MessageError reply(msg, "error.marshalling", DBUS_ERROR_FAILED);
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


void
listen(DBus::Connection& conn)
{
    y2mil("Listening for method calls and signals");

    conn.request_name("org.opensuse.snapper", DBUS_NAME_FLAG_REPLACE_EXISTING);

    conn.add_match("type='signal', interface='" DBUS_INTERFACE_DBUS "', member='NameOwnerChanged'");

    int idle = 0;
    while (++idle < 1000 || !clients.empty() || !jobs.empty())
    {
	conn.read_write(100);	// TODO

	jobs.handle();

	DBusMessage* tmp = dbus_connection_pop_message(conn.get_connection());
	if (!tmp)
	    continue;

	DBus::Message msg(tmp);

	switch (msg.get_type())
	{
	    case DBUS_MESSAGE_TYPE_METHOD_CALL:
	    {
		y2mil("method call sender:'" << msg.get_sender() << "' path:'" <<
		      msg.get_path() << "' interface:'" << msg.get_interface() <<
		      "' member:'" << msg.get_member() << "'");

		if (msg.is_method_call(DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
		{
		    reply_to_introspect(conn, msg);
		    break;
		}

		if (clients.find(msg.get_sender()) == clients.end())
		{
		    y2mil("client connected invisible '" << msg.get_sender() << "'");
		    client_connected(msg.get_sender());
		}

		dispatch(conn, msg);
	    }
	    break;

	    case DBUS_MESSAGE_TYPE_SIGNAL:
	    {
		y2mil("signal sender:'" << msg.get_sender() << "' path:'" <<
		      msg.get_path() << "' interface:'" << msg.get_interface() <<
		      "' member:'" << msg.get_member() << "'");

		if (msg.is_signal(DBUS_INTERFACE_DBUS, "NameOwnerChanged"))
		{
		    string name, old_owner, new_owner;

		    DBus::Hihi hihi(msg);
		    hihi >> name >> old_owner >> new_owner;

		    if (name == new_owner && old_owner.empty())
		    {
			y2mil("client connected '" << name << "'");
			client_connected(name);
		    }

		    if (name == old_owner && new_owner.empty())
		    {
			y2mil("client disconnected '" << name << "'");
			client_disconnected(name);
		    }
		}
	    }
	    break;
	}

	idle = 0;
    }

    y2mil("Idle - exiting");
}


int
main(int argc, char** argv)
{
    initDefaultLogger();

    DBus::Connection conn(DBUS_BUS_SYSTEM);

    listen(conn);

    return 0;
}
