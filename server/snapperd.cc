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


#include <stdlib.h>
#include <iostream>
#include <string>

#include <snapper/Log.h>
#include <dbus/DBusMainLoop.h>

#include "MetaSnapper.h"
#include "Client.h"
#include "Types.h"


using namespace std;


const int idle_time = 60;
const int snapper_cleanup_time = 30;


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
    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    clients.remove_zombies();

    if (clients.empty())
	set_idle_timeout(idle_time);

    for (MetaSnappers::iterator it = meta_snappers.begin(); it != meta_snappers.end(); ++it)
    {
	if (it->is_loaded() && it->unused_for() > snapper_cleanup_time)
	    it->unload();
    }
}


int
MyMainLoop::periodic_timeout()
{
    boost::unique_lock<boost::shared_mutex> lock(big_mutex);

    if (clients.has_zombies())
	return 1000;

    for (MetaSnappers::const_iterator it = meta_snappers.begin(); it != meta_snappers.end(); ++it)
	if (it->is_loaded() && it->use_count() == 0)
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

    dbus_threads_init_default();

    MyMainLoop mainloop(DBUS_BUS_SYSTEM);

    mainloop.set_idle_timeout(idle_time);

    y2mil("Requesting DBus name");

    mainloop.request_name(SERVICE, DBUS_NAME_FLAG_REPLACE_EXISTING);

    y2mil("Loading snapper configs");

    meta_snappers.init();

    y2mil("Listening for method calls and signals");

    mainloop.run();

    y2mil("Exiting");

    return 0;
}
