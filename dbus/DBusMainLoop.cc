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
#include <poll.h>
#include <time.h>

#include "DBusMainLoop.h"


namespace DBus
{

    MainLoop::MainLoop(DBusBusType type)
	: Connection(type), idle_timeout(-1)
    {
	if (pipe(wakeup_pipe) != 0)
	    throw FatalException();

	if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch, toggled_watch,
						 this, NULL))
	    throw FatalException();

	if (!dbus_connection_set_timeout_functions(conn, add_timeout, remove_timeout,
						   toggled_timeout, this, NULL))
	    throw FatalException();

	dbus_connection_set_wakeup_main_function(conn, wakeup_main, this, NULL);

	// Filtering for the sender doesn't work for me. So also check the
	// sender later when handling the signal.
	add_match("type='signal', sender='" DBUS_SERVICE_DBUS "', path='" DBUS_PATH_DBUS "', "
		  "interface='" DBUS_INTERFACE_DBUS "', member='NameOwnerChanged'");
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

	    int timeout = periodic_timeout();

	    if (idle_timeout >= 0)
	    {
		int time_left = last_action - monotonic_clock() + idle_timeout;

		if (timeout > time_left * 1000 || timeout == -1)
		    timeout = time_left * 1000;
	    }

	    int r = poll(&pollfds[0], pollfds.size(), timeout);
	    if (r == -1)
		throw FatalException();

	    periodic();

	    for (vector<struct pollfd>::const_iterator it2 = pollfds.begin(); it2 != pollfds.end(); ++it2)
	    {
		if (it2->fd == wakeup_pipe[0] && (it2->revents & POLLIN))
		{
		    char arbitrary;
		    read(wakeup_pipe[0], &arbitrary, 1);
		}
	    }

	    for (vector<Watch>::const_iterator it = watches.begin(); it != watches.end(); ++it)
	    {
		if (it->enabled)
		{
		    for (vector<struct pollfd>::const_iterator it2 = pollfds.begin(); it2 != pollfds.end(); ++it2)
		    {
			if (it2->fd == it->fd)
			{
			    unsigned int flags = 0;

			    if (it2->revents & POLLIN)
				flags |= DBUS_WATCH_READABLE;

			    if (it2->revents & POLLOUT)
				flags |= DBUS_WATCH_WRITABLE;

			    if (flags != 0)
			    {
				boost::lock_guard<boost::mutex> lock(mutex);
				dbus_watch_handle(it->dbus_watch, flags);
			    }
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

	    if (idle_timeout >= 0)
	    {
		int time_left = last_action - monotonic_clock() + idle_timeout;

		if (time_left <= 0)
		    break;
	    }
	}
    }


    void
    MainLoop::set_idle_timeout(int s)
    {
	idle_timeout = s;
    }


    void
    MainLoop::reset_idle_count()
    {
	last_action = monotonic_clock();
    }


    vector<MainLoop::Watch>::iterator
    MainLoop::find_watch(DBusWatch* dbus_watch)
    {
	for (vector<Watch>::iterator it = watches.begin(); it != watches.end(); ++it)
	    if (it->dbus_watch == dbus_watch)
		return it;

	throw FatalException();
    }


    vector<MainLoop::Timeout>::iterator
    MainLoop::find_timeout(DBusTimeout* dbus_timeout)
    {
	for (vector<Timeout>::iterator it = timeouts.begin(); it != timeouts.end(); ++it)
	    if (it->dbus_timeout == dbus_timeout)
		return it;

	throw FatalException();
    }


    dbus_bool_t
    MainLoop::add_watch(DBusWatch* dbus_watch, void* data)
    {
	Watch tmp;
	tmp.enabled = dbus_watch_get_enabled(dbus_watch);
	tmp.fd = dbus_watch_get_unix_fd(dbus_watch);
	tmp.flags = dbus_watch_get_flags(dbus_watch);

	tmp.events = 0;
	if (tmp.flags & DBUS_WATCH_READABLE)
	    tmp.events |= POLLIN;
	if (tmp.flags & DBUS_WATCH_WRITABLE)
	    tmp.events |= POLLOUT;

	tmp.dbus_watch = dbus_watch;

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
	Timeout tmp;
	tmp.enabled = dbus_timeout_get_enabled(dbus_timeout);
	tmp.interval = dbus_timeout_get_interval(dbus_timeout);
	tmp.dbus_timeout = dbus_timeout;

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

		    DBus::Hihi hihi(msg);
		    hihi >> name >> old_owner >> new_owner;

		    if (name == new_owner && old_owner.empty())
			client_connected(name);

		    if (name == old_owner && new_owner.empty())
			client_disconnected(name);
		}
	    }
	    break;
	}
    }


    time_t
    DBus::MainLoop::monotonic_clock()
    {
	struct timespec tmp;
	clock_gettime(CLOCK_MONOTONIC, &tmp);
	return tmp.tv_sec;
    }

}
