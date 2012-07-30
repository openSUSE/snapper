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


#include "Client.h"
#include "MetaSnapper.h"


template <typename ListType, typename Type>
bool contains(const ListType& l, const Type& value)
{
    return find(l.begin(), l.end(), value) != l.end();
}


Client::Client(const string& name)
    : name(name)
{
}


Client::~Client()
{
    thread.interrupt();

    thread.join();		// TODO this can block

    for (list<Comparison*>::iterator it = comparisons.begin(); it != comparisons.end(); ++it)
    {
	delete *it;
    }
}


Comparison*
Client::find_comparison(Snapper* snapper, Snapshots::const_iterator snapshot1,
			Snapshots::const_iterator snapshot2)
{
    for (list<Comparison*>::iterator it = comparisons.begin(); it != comparisons.end(); ++it)
    {
	if ((*it)->getSnapper() == snapper && (*it)->getSnapshot1() == snapshot1 &&
	    (*it)->getSnapshot2() == snapshot2)
	    return *it;
    }

    throw NoComparison();
}


Comparison*
Client::find_comparison(Snapper* snapper, unsigned int number1, unsigned int number2)
{
    Snapshots& snapshots = snapper->getSnapshots();
    Snapshots::const_iterator snapshot1 = snapshots.find(number1);
    Snapshots::const_iterator snapshot2 = snapshots.find(number2);

    return find_comparison(snapper, snapshot1, snapshot2);
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

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if GCC_VERSION > 40500
    entries.emplace_back(name);
#else
    entries.push_back(name);
#endif

#undef GCC_VERSION

    return --entries.end();
}


void
Clients::remove(const string& name)
{
    iterator it = find(name);
    if (it != entries.end())
	entries.erase(it);
}
