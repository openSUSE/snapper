/*
 * Copyright (c) [2012-2015] Novell, Inc.
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
#include <poll.h>

#include "DBusMainLoop.h"


namespace DBus
{

    MainLoop::Watch::Watch(DBusWatch* dbus_watch)
	: dbus_watch(dbus_watch), enabled(false), fd(-1), events(0)
    {
	enabled = dbus_watch_get_enabled(dbus_watch);
	fd = dbus_watch_get_unix_fd(dbus_watch);

	unsigned int flags = dbus_watch_get_flags(dbus_watch);
	if (flags & DBUS_WATCH_READABLE)
	    events |= POLLIN;
	if (flags & DBUS_WATCH_WRITABLE)
	    events |= POLLOUT;
    }


    MainLoop::Timeout::Timeout(DBusTimeout* dbus_timeout)
	: dbus_timeout(dbus_timeout), enabled(false), interval(0)
    {
	enabled = dbus_timeout_get_enabled(dbus_timeout);
	interval = dbus_timeout_get_interval(dbus_timeout);
    }


    MainLoop::MainLoop(DBusBusType type)
	: Connection(type), idle_timeout(-1)
    {
	if (pipe(wakeup_pipe) != 0)
	    SN_THROW(FatalException());

	if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch, toggled_watch,
						 this, NULL))
	    SN_THROW(FatalException());

	if (!dbus_connection_set_timeout_functions(conn, add_timeout, remove_timeout,
						   toggled_timeout, this, NULL))
	    SN_THROW(FatalException());

	dbus_connection_set_wakeup_main_function(conn, wakeup_main, this, NULL);
    }


    MainLoop::~MainLoop()
    {
	close(wakeup_pipe[0]);
	close(wakeup_pipe[1]);
    }


    void
    DBus::MainLoop::run()
    {
	reset_idle_count();

	while (true)
	{
	    vector<struct pollfd> pollfds;

	    {
		struct pollfd tmp;
		tmp.fd = wakeup_pipe[0];
		tmp.events = POLLIN;
		tmp.revents = 0;
		pollfds.push_back(tmp);
	    }

	    for (vector<Watch>::const_iterator it = watches.begin(); it != watches.end(); ++it)
	    {
		if (it->enabled)
		{
		    struct pollfd tmp;
		    tmp.fd = it->fd;
		    tmp.events = it->events;
		    tmp.revents = 0;
		    pollfds.push_back(tmp);
		}
	    }

	    milliseconds timeout = periodic_timeout();

	    if (idle_timeout.count() >= 0)
	    {
		steady_clock::duration time_left = idle_for() + idle_timeout;
		if (timeout > time_left || timeout.count() < 0)
		    timeout = duration_cast<milliseconds>(time_left);
	    }

	    int r = poll(&pollfds[0], pollfds.size(), timeout.count());
	    if (r == -1)
		SN_THROW(FatalException());

	    periodic();

	    for (vector<struct pollfd>::const_iterator it1 = pollfds.begin(); it1 != pollfds.end(); ++it1)
	    {
		if (it1->fd == wakeup_pipe[0])
		{
		    if (it1->revents & POLLIN)
		    {
			char arbitrary;
			read(wakeup_pipe[0], &arbitrary, 1);
		    }
		}
		else
		{
		    unsigned int flags = 0;

		    if (it1->revents & POLLIN)
			flags |= DBUS_WATCH_READABLE;

		    if (it1->revents & POLLOUT)
			flags |= DBUS_WATCH_WRITABLE;

		    if (flags != 0)
		    {
			// Do not iterate over watches here since calling dbus_watch_handle() can
			// trigger a remove_watch() callback, thus invalidating the iterator.
			// Instead always search for the watch.

			vector<Watch>::const_iterator it2 = find_enabled_watch(it1->fd, it1->events);
			if (it2 != watches.end())
			{
			    boost::lock_guard<boost::mutex> lock(mutex);
			    dbus_watch_handle(it2->dbus_watch, flags);
			}
		    }
		}
	    }

	    DBusMessage* tmp = pop_message();
	    if (tmp)
	    {
		DBus::Message msg(tmp, false);
		dispatch_incoming(msg);
	    }

	    if (idle_timeout.count() >= 0)
	    {
		steady_clock::duration time_left = idle_for() + idle_timeout;
		if (time_left.count() <= 0)
		    break;
	    }
	}
    }


    void
    MainLoop::set_idle_timeout(milliseconds idle_timeout)
    {
	MainLoop::idle_timeout = idle_timeout;
    }


    void
    MainLoop::reset_idle_count()
    {
	last_action = steady_clock::now();
    }


    milliseconds
    MainLoop::idle_for() const
    {
	return duration_cast<milliseconds>(last_action - steady_clock::now());
    }


    vector<MainLoop::Watch>::iterator
    MainLoop::find_watch(DBusWatch* dbus_watch)
    {
	for (vector<Watch>::iterator it = watches.begin(); it != watches.end(); ++it)
	    if (it->dbus_watch == dbus_watch)
		return it;

	SN_THROW(FatalException());
	__builtin_unreachable();
    }


    vector<MainLoop::Watch>::iterator
    MainLoop::find_enabled_watch(int fd, short events)
    {
	for (vector<Watch>::iterator it = watches.begin(); it != watches.end(); ++it)
	    if (it->enabled && it->fd == fd && it->events == events)
		return it;

	return watches.end();
    }


    vector<MainLoop::Timeout>::iterator
    MainLoop::find_timeout(DBusTimeout* dbus_timeout)
    {
	for (vector<Timeout>::iterator it = timeouts.begin(); it != timeouts.end(); ++it)
	    if (it->dbus_timeout == dbus_timeout)
		return it;

	SN_THROW(FatalException());
	__builtin_unreachable();
    }


    dbus_bool_t
    MainLoop::add_watch(DBusWatch* dbus_watch, void* data)
    {
	Watch tmp(dbus_watch);
	MainLoop* s = static_cast<MainLoop*>(data);
	s->watches.push_back(tmp);
	return true;
    }


    void
    MainLoop::remove_watch(DBusWatch* dbus_watch, void* data)
    {
	MainLoop* s = static_cast<MainLoop*>(data);
	vector<Watch>::iterator it = s->find_watch(dbus_watch);
	s->watches.erase(it);
    }


    void
    MainLoop::toggled_watch(DBusWatch* dbus_watch, void* data)
    {
	MainLoop* s = static_cast<MainLoop*>(data);
	vector<Watch>::iterator it = s->find_watch(dbus_watch);
	it->enabled = dbus_watch_get_enabled(dbus_watch);
    }


    dbus_bool_t
    MainLoop::add_timeout(DBusTimeout* dbus_timeout, void* data)
    {
	Timeout tmp(dbus_timeout);
	MainLoop* s = static_cast<MainLoop*>(data);
	s->timeouts.push_back(tmp);
	return true;
    }


    void
    MainLoop::remove_timeout(DBusTimeout* dbus_timeout, void* data)
    {
	MainLoop* s = static_cast<MainLoop*>(data);
	vector<Timeout>::iterator it = s->find_timeout(dbus_timeout);
	s->timeouts.erase(it);
    }


    void
    MainLoop::toggled_timeout(DBusTimeout* dbus_timeout, void* data)
    {
	MainLoop* s = static_cast<MainLoop*>(data);
	vector<Timeout>::iterator it = s->find_timeout(dbus_timeout);
	it->enabled = dbus_timeout_get_enabled(dbus_timeout);
    }


    void
    MainLoop::wakeup_main(void* data)
    {
	MainLoop* s = static_cast<MainLoop*>(data);
	const char arbitrary = 42;
	write(s->wakeup_pipe[1], &arbitrary, 1);
    }


    void
    DBus::MainLoop::add_client_match(const string& name)
    {
	// Filtering for the sender doesn't work for me. So also check the
	// sender later when handling the signal.
	add_match("type='signal', sender='" DBUS_SERVICE_DBUS "', path='" DBUS_PATH_DBUS "', "
		  "interface='" DBUS_INTERFACE_DBUS "', member='NameOwnerChanged', "
		  "arg0='" + name + "'");
    }


    void
    DBus::MainLoop::remove_client_match(const string& name)
    {
	// Filtering for the sender doesn't work for me. So also check the
	// sender later when handling the signal.
	remove_match("type='signal', sender='" DBUS_SERVICE_DBUS "', path='" DBUS_PATH_DBUS "', "
		     "interface='" DBUS_INTERFACE_DBUS "', member='NameOwnerChanged', "
		     "arg0='" + name + "'");
    }


    void
    DBus::MainLoop::dispatch_incoming(Message& msg)
    {
	switch (msg.get_type())
	{
	    case DBUS_MESSAGE_TYPE_METHOD_CALL:
	    {
		method_call(msg);
	    }
	    break;

	    case DBUS_MESSAGE_TYPE_SIGNAL:
	    {
		signal(msg);

		if (msg.get_sender() == DBUS_SERVICE_DBUS &&
		    msg.is_signal(DBUS_INTERFACE_DBUS, "NameOwnerChanged"))
		{
		    string name, old_owner, new_owner;

		    DBus::Unmarshaller unmarshaller(msg);
		    unmarshaller >> name >> old_owner >> new_owner;

		    if (name == old_owner && new_owner.empty())
			client_disconnected(name);
		}
	    }
	    break;
	}
    }

}
