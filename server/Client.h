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


#ifndef SNAPPER_CLIENT_H
#define SNAPPER_CLIENT_H


#include <string>
#include <list>
#include <queue>
#include <set>
#include <boost/thread.hpp>

#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/Factory.h>
#include <snapper/Comparison.h>

#include "dbus/DBusConnection.h"
#include "dbus/DBusMessage.h"

using namespace std;
using namespace snapper;


struct NoComparison : public std::exception
{
    explicit NoComparison() throw() {}
    virtual const char* what() const throw() { return "no comparison"; }
};


class Commands
{
public:

    void list_configs(DBus::Connection& conn, DBus::Message& msg);
    void get_config(DBus::Connection& conn, DBus::Message& msg);
    void create_config(DBus::Connection& conn, DBus::Message& msg);
    void delete_config(DBus::Connection& conn, DBus::Message& msg);
    void lock_config(DBus::Connection& conn, DBus::Message& msg);
    void unlock_config(DBus::Connection& conn, DBus::Message& msg);
    void list_snapshots(DBus::Connection& conn, DBus::Message& msg);
    void get_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void set_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void create_single_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void create_pre_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void create_post_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void delete_snapshots(DBus::Connection& conn, DBus::Message& msg);
    void mount_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void umount_snapshot(DBus::Connection& conn, DBus::Message& msg);
    void create_comparison(DBus::Connection& conn, DBus::Message& msg);
    void get_files(DBus::Connection& conn, DBus::Message& msg);
    void get_diff(DBus::Connection& conn, DBus::Message& msg);
    void set_undo(DBus::Connection& conn, DBus::Message& msg);
    void set_undo_all(DBus::Connection& conn, DBus::Message& msg);
    void get_undo_steps(DBus::Connection& conn, DBus::Message& msg);
    void do_undo_step(DBus::Connection& conn, DBus::Message& msg);
    void debug(DBus::Connection& conn, DBus::Message& msg);

    void dispatch(DBus::Connection& conn, DBus::Message& msg);

};


class Client : public Commands
{
public:

    Client(const string& name);
    ~Client();

    Comparison* find_comparison(const string& config_name, unsigned int number1,
				unsigned int number2);

    Comparison* find_comparison(Snapper* snapper, Snapshots::const_iterator snapshot1,
				Snapshots::const_iterator snapshot2);

    void add_lock(const string& config_name);
    void remove_lock(const string& config_name);
    bool has_lock(const string& config_name) const;

    string name;

    list<Comparison*> comparisons;

    set<string> locks;

    struct Task
    {
	Task(DBus::Connection& conn, DBus::Message& msg) : conn(conn), msg(msg) {}

	DBus::Connection& conn;
	DBus::Message msg;
    };

    boost::condition_variable* c;
    boost::mutex* m;
    boost::thread* t;

    queue<Task> tasks;

    void add_task(DBus::Connection& conn, DBus::Message& msg);

    void worker();

};


class Clients
{
public:

    typedef list<Client>::iterator iterator;
    typedef list<Client>::const_iterator const_iterator;

    iterator begin() { return entries.begin(); }
    const_iterator begin() const { return entries.begin(); }

    iterator end() { return entries.end(); }
    const_iterator end() const { return entries.end(); }

    bool empty() const { return entries.empty(); }

    iterator find(const string& name);

    iterator add(const string& name);
    void remove(const string& name);

private:

    list<Client> entries;

};


#endif
