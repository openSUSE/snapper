/*
 * Copyright (c) [2012-2015] Novell, Inc.
 * Copyright (c) [2016,2018] SUSE LLC
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


#ifndef SNAPPER_CLIENT_H
#define SNAPPER_CLIENT_H


#include <string>
#include <list>
#include <queue>
#include <set>
#include <boost/thread.hpp>

#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/Comparison.h>
#include <dbus/DBusConnection.h>
#include <dbus/DBusMessage.h>

#include "MetaSnapper.h"


using namespace std;
using namespace snapper;


#define SERVICE "org.opensuse.Snapper"
#define PATH "/org/opensuse/Snapper"
#define INTERFACE "org.opensuse.Snapper"


extern boost::shared_mutex big_mutex;

class Backgrounds;
class Clients;


struct NoComparison : Exception
{
    explicit NoComparison() : Exception("no comparison") {}
};


class Client : private boost::noncopyable
{
public:

    static void introspect(DBus::Connection& conn, DBus::Message& msg);

    void check_permission(DBus::Connection& conn, DBus::Message& msg) const;
    void check_permission(DBus::Connection& conn, DBus::Message& msg,
			  const MetaSnapper& meta_snapper) const;
    void check_lock(DBus::Connection& conn, DBus::Message& msg, const string& config_name) const;
    void check_config_in_use(const MetaSnapper& meta_snapper) const;
    void check_snapshot_in_use(const MetaSnapper& meta_snapper, unsigned int number) const;

    void signal_config_created(DBus::Connection& conn, const string& config_name);
    void signal_config_modified(DBus::Connection& conn, const string& config_name);
    void signal_config_deleted(DBus::Connection& conn, const string& config_name);
    void signal_snapshot_created(DBus::Connection& conn, const string& config_name,
				 unsigned int num);
    void signal_snapshot_modified(DBus::Connection& conn, const string& config_name,
				  unsigned int num);
    void signal_snapshots_deleted(DBus::Connection& conn, const string& config_name,
				  const list<dbus_uint32_t>& nums);

    void list_configs(DBus::Connection& conn, DBus::Message& msg);
    void get_config(DBus::Connection& conn, DBus::Message& msg);
    void set_config(DBus::Connection& conn, DBus::Message& msg);
    void create_config(DBus::Connection& conn, DBus::Message& msg);
    void delete_config(DBus::Connection& conn, DBus::Message& msg);
    void lock_config(DBus::Connection& conn, DBus::Message& msg);
    void unlock_config(DBus::Connection& conn, DBus::Message& msg);
    void list_snapshots(DBus::Connection& conn, DBus::Message& msg);
    void list_snapshots_at_time(DBus::Connection& conn, DBus::Message& msg);
    void get_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void set_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void create_single_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void create_single_snapshot_v2(DBus::Connection& conn, DBus::Message& msg);
    void create_single_snapshot_of_default(DBus::Connection& conn, DBus::Message& msg);
    void create_pre_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void create_post_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void delete_snapshots(DBus::Connection& conn, DBus::Message& msg);
    void get_default_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void get_active_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void calculate_used_space(DBus::Connection& conn, DBus::Message& msg);
    void get_used_space(DBus::Connection& conn, DBus::Message& msg);
    void mount_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void umount_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void get_mount_point(DBus::Connection& conn, DBus::Message& msg);
    void create_comparison(DBus::Connection& conn, DBus::Message& msg);
    void delete_comparison(DBus::Connection& conn, DBus::Message& msg);
    void get_files(DBus::Connection& conn, DBus::Message& msg);
    void setup_quota(DBus::Connection& conn, DBus::Message& msg);
    void prepare_quota(DBus::Connection& conn, DBus::Message& msg);
    void query_quota(DBus::Connection& conn, DBus::Message& msg);
    void query_free_space(DBus::Connection& conn, DBus::Message& msg);
    void sync(DBus::Connection& conn, DBus::Message& msg);
    void debug(DBus::Connection& conn, DBus::Message& msg) const;

    void dispatch(DBus::Connection& conn, DBus::Message& msg);

    Client(const string& name, const Clients& clients);
    ~Client();

    list<Comparison*>::iterator find_comparison(Snapper* snapper, unsigned int number1,
						unsigned int number2);

    list<Comparison*>::iterator find_comparison(Snapper* snapper,
						Snapshots::const_iterator snapshot1,
						Snapshots::const_iterator snapshot2);

    void delete_comparison(list<Comparison*>::iterator);

    void add_lock(const string& config_name);
    void remove_lock(const string& config_name);
    bool has_lock(const string& config_name) const;

    void add_mount(const string& config_name, unsigned int number);
    void remove_mount(const string& config_name, unsigned int number);

    const string name;

    list<Comparison*> comparisons;

    set<string> locks;

    map<pair<string, unsigned int>, unsigned int> mounts;

    struct Task
    {
	Task(DBus::Connection& conn, DBus::Message& msg) : conn(conn), msg(msg) {}

	DBus::Connection& conn;
	DBus::Message msg;
    };

    boost::condition_variable condition;
    boost::mutex mutex;
    boost::thread thread;
    queue<Task> tasks;

    bool zombie = false;

    void add_task(DBus::Connection& conn, DBus::Message& msg);

private:

    void worker();

    const Clients& clients;

};


class Clients
{
public:

    Clients(Backgrounds& backgrounds);

    typedef list<Client>::iterator iterator;
    typedef list<Client>::const_iterator const_iterator;

    iterator begin() { return entries.begin(); }
    const_iterator begin() const { return entries.begin(); }

    iterator end() { return entries.end(); }
    const_iterator end() const { return entries.end(); }

    bool empty() const { return entries.empty(); }

    iterator find(const string& name);

    iterator add(const string& name);
    void remove_zombies();

    bool has_zombies() const;

    Backgrounds& backgrounds() const;

private:

    list<Client> entries;

    Backgrounds& bgs;

};


#endif
