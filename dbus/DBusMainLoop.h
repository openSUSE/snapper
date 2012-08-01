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


#ifndef SNAPPER_DBUSMAINLOOP_H
#define SNAPPER_DBUSMAINLOOP_H


#include <dbus/dbus.h>

#include "DBusConnection.h"


namespace DBus
{

    class MainLoop : public Connection
    {
    public:

	MainLoop(DBusBusType type);
	virtual ~MainLoop();

	void run();

	void set_idle_timeout(int s);
	void reset_idle_count();

	virtual void method_call(Message& message) = 0;
	virtual void signal(Message& message) = 0;
	virtual void client_connected(const string& name) = 0;
	virtual void client_disconnected(const string& name) = 0;
	virtual int periodic_timeout() = 0;
	virtual void periodic() = 0;

    protected:

	struct Watch
	{
	    bool enabled;
	    int fd;
	    unsigned int flags;
	    int events;
	    DBusWatch* dbus_watch;
	};

	struct Timeout
	{
	    bool enabled;
	    int interval;
	    DBusTimeout* dbus_timeout;
	};

	vector<Watch> watches;
	vector<Timeout> timeouts;
	int wakeup_pipe[2];

	vector<Watch>::iterator find_watch(DBusWatch* dbus_watch);
	vector<Timeout>::iterator find_timeout(DBusTimeout* dbus_timeout);

	static dbus_bool_t add_watch(DBusWatch* dbus_watch, void* data);
	static void remove_watch(DBusWatch* dbus_watch, void* data);
	static void toggled_watch(DBusWatch* dbus_watch, void* data);

	static dbus_bool_t add_timeout(DBusTimeout* dbus_timeout, void* data);
	static void remove_timeout(DBusTimeout* dbus_timeout, void* data);
	static void toggled_timeout(DBusTimeout* dbus_timeout, void* data);

	static void wakeup_main(void* data);

	void dispatch_incoming(Message& message);

	int idle_timeout;
	time_t last_action;

    };

}


#endif
