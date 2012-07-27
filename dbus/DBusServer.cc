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


#include <poll.h>
#include <time.h>

#include "DBusServer.h"


namespace DBus
{

    Server::Server(DBusBusType type)
	: Connection(type), idle_timeout(-1)
    {
	if (pipe(wakeup_pipe) != 0)
	    throw FatalException();

	if (!dbus_connection_set_watch_functions(get_connection(), add_watch, remove_watch,
						 toggled_watch, this, NULL))
	    throw FatalException();

	if (!dbus_connection_set_timeout_functions(get_connection(), add_timeout, remove_timeout,
						   toggled_timeout, this, NULL))
	    throw FatalException();

	dbus_connection_set_wakeup_main_function(get_connection(), wakeup_main, this, NULL);
    }


    Server::~Server()
    {
	close(wakeup_pipe[0]);
	close(wakeup_pipe[1]);
    }


    void
    DBus::Server::run()
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

	    int timeout = -1;

	    if (idle_timeout >= 0)
	    {
		struct timespec tmp;
		clock_gettime(CLOCK_MONOTONIC, &tmp);
		int time_left = last_action - tmp.tv_sec + idle_timeout;

		if (timeout > time_left * 1000 || timeout == -1)
		    timeout = time_left * 1000;
	    }

	    int r = poll(&pollfds[0], pollfds.size(), timeout);
	    if (r == -1)
		throw FatalException();

	    {
		for (vector<struct pollfd>::const_iterator it2 = pollfds.begin(); it2 != pollfds.end(); ++it2)
		{
		    if (it2->fd == wakeup_pipe[0] && (it2->revents & POLLIN))
		    {
			char arbitrary;
			read(wakeup_pipe[0], &arbitrary, 1);
		    }
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
			    if ((it2->revents & POLLIN) && (it->flags & DBUS_WATCH_READABLE))
				dbus_watch_handle(it->dbus_watch, DBUS_WATCH_READABLE);

			    if ((it2->revents & POLLOUT) && (it->flags & DBUS_WATCH_WRITABLE))
				dbus_watch_handle(it->dbus_watch, DBUS_WATCH_WRITABLE);
			}
		    }
		}
	    }

	    while (dbus_connection_get_dispatch_status(get_connection()) == DBUS_DISPATCH_DATA_REMAINS)
	    {
		dbus_connection_dispatch(get_connection());
	    }

	    if (idle_timeout >= 0)
	    {
		struct timespec tmp;
		clock_gettime(CLOCK_MONOTONIC, &tmp);
		int time_left = last_action - tmp.tv_sec + idle_timeout;

		if (time_left <= 0)
		    break;
	    }
	}
    }


    void
    Server::set_idle_timeout(int s)
    {
	idle_timeout = s;
    }


    void
    Server::reset_idle_count()
    {
	struct timespec tmp;
	clock_gettime(CLOCK_MONOTONIC, &tmp);
	last_action = tmp.tv_sec;
    }


    vector<Server::Watch>::iterator
    Server::find_watch(DBusWatch* dbus_watch)
    {
	for (vector<Watch>::iterator it = watches.begin(); it != watches.end(); ++it)
	    if (it->dbus_watch == dbus_watch)
		return it;

	throw FatalException();
    }


    vector<Server::Timeout>::iterator
    Server::find_timeout(DBusTimeout* dbus_timeout)
    {
	for (vector<Timeout>::iterator it = timeouts.begin(); it != timeouts.end(); ++it)
	    if (it->dbus_timeout == dbus_timeout)
		return it;

	throw FatalException();
    }


    dbus_bool_t
    Server::add_watch(DBusWatch* dbus_watch, void* data)
    {
	Server* s = static_cast<Server*>(data);

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
	s->watches.push_back(tmp);

	return true;
    }


    void
    Server::remove_watch(DBusWatch* dbus_watch, void* data)
    {
	Server* s = static_cast<Server*>(data);

	vector<Watch>::iterator it = s->find_watch(dbus_watch);
	s->watches.erase(it);
    }


    void
    Server::toggled_watch(DBusWatch* dbus_watch, void* data)
    {
	Server* s = static_cast<Server*>(data);

	vector<Watch>::iterator it = s->find_watch(dbus_watch);
	it->enabled = dbus_watch_get_enabled(dbus_watch);
    }


    dbus_bool_t
    Server::add_timeout(DBusTimeout* dbus_timeout, void* data)
    {
	Server* s = static_cast<Server*>(data);

	Timeout tmp;
	tmp.enabled = dbus_timeout_get_enabled(dbus_timeout);
	tmp.interval = dbus_timeout_get_interval(dbus_timeout);
	tmp.dbus_timeout = dbus_timeout;
	s->timeouts.push_back(tmp);

	return true;
    }


    void
    Server::remove_timeout(DBusTimeout* dbus_timeout, void* data)
    {
	Server* s = static_cast<Server*>(data);

	vector<Timeout>::iterator it = s->find_timeout(dbus_timeout);
	s->timeouts.erase(it);
    }


    void
    Server::toggled_timeout(DBusTimeout* dbus_timeout, void* data)
    {
	Server* s = static_cast<Server*>(data);

	vector<Timeout>::iterator it = s->find_timeout(dbus_timeout);
	it->enabled = dbus_timeout_get_enabled(dbus_timeout);
    }


    void
    Server::wakeup_main(void* data)
    {
	Server* s = static_cast<Server*>(data);

	const char arbitrary = 42;
	write(s->wakeup_pipe[1], &arbitrary, 1);
    }

}
