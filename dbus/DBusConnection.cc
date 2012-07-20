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
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>

#include "DBusConnection.h"

namespace DBus
{

    Connection::Connection(DBusBusType type)
    {
	DBusError err;
	dbus_error_init(&err);

	conn = dbus_bus_get(type, &err);
	if (dbus_error_is_set(&err))
	{
	    dbus_error_free(&err);
	    throw FatalException();
	}

	if (!conn)
	{
	    throw FatalException();
	}
    }


    Connection::~Connection()
    {
	dbus_connection_unref(conn);
    }


    void
    Connection::request_name(const char* name, unsigned int flags)
    {
	boost::lock_guard<boost::mutex> lock(mutex);

	DBusError err;
	dbus_error_init(&err);

	int ret = dbus_bus_request_name(conn, name, flags, &err);
	if (dbus_error_is_set(&err))
	{
	    dbus_error_free(&err);
	    throw FatalException();
	}

	if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	{
	    throw FatalException();
	}
    }


    void
    Connection::read_write(int timeout)
    {
	// TODO

	dbus_connection_read_write(conn, timeout);
    }


    void
    Connection::send(Message& m)
    {
	boost::lock_guard<boost::mutex> lock(mutex);

	if (!dbus_connection_send(conn, m.get_message(), NULL))
	{
	    throw FatalException();
	}
    }


    Message
    Connection::send_with_reply_and_block(Message& m)
    {
	boost::lock_guard<boost::mutex> lock(mutex);

	DBusError err;
	dbus_error_init(&err);

	DBusMessage* tmp = dbus_connection_send_with_reply_and_block(conn, m.get_message(),
								     0x7fffffff, &err);
	if (dbus_error_is_set(&err))
	{
	    throw ErrorException(err);
	}

	Message reply(tmp);
	return reply;
    }


    void
    Connection::add_match(const char* rule)
    {
	boost::lock_guard<boost::mutex> lock(mutex);

	DBusError err;
	dbus_error_init(&err);

	dbus_bus_add_match(conn, rule, &err);

	if (dbus_error_is_set(&err))
	{
	    dbus_error_free(&err);
	    throw FatalException();
	}
    }


    unsigned long
    Connection::get_unix_userid(const Message& m)
    {
	boost::lock_guard<boost::mutex> lock(mutex);

	string sender = m.get_sender();
	if (sender.empty())
	{
	    throw FatalException();
	}

	DBusError err;
	dbus_error_init(&err);

	unsigned long uid = dbus_bus_get_unix_user(conn, sender.c_str(), &err);
	if (dbus_error_is_set(&err))
	{
	    dbus_error_free(&err);
	    throw FatalException();
	}

	return uid;
    }


    string
    Connection::get_unix_username(unsigned long userid)
    {
	long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
	char* buf = (char*) malloc(bufsize);
	if (!buf)
	    throw FatalException();

	struct passwd pwd;
	struct passwd *result;
	getpwuid_r(userid, &pwd, buf, bufsize, &result);
	if (!result)
	    throw FatalException();

	string username(pwd.pw_name);

	free(buf);

	return username;
    }

}
