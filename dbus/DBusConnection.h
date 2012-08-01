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


#ifndef SNAPPER_DBUSCONNECTION_H
#define SNAPPER_DBUSCONNECTION_H


#include <dbus/dbus.h>

#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>

#include "DBusMessage.h"


namespace DBus
{

    class Connection : boost::noncopyable
    {
    public:

	Connection(DBusBusType type);
	~Connection();

	void request_name(const char* name, unsigned int flags);

	void send(Message& m);

	Message send_with_reply_and_block(Message& m);

	void add_match(const char* rule);

	unsigned long get_unix_userid(const Message& m);

    protected:

	// Without locking the connection manually the server sometimes does
	// not reply to request.  Esp. the lock around dbus_watch_handle() is
	// required.
	boost::mutex mutex;

	DBusConnection* conn;

	DBusMessage* pop_message();

    };

}


#endif
