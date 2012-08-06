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


#include <assert.h>
#include <stdlib.h>

#include "DBusMessage.h"


namespace DBus
{

    Message::Message(DBusMessage* m, bool ref)
	: msg(m)
    {
	assert(msg);
	if (ref)
	    dbus_message_ref(msg);
    }


    Message::Message(const Message& m)
	: msg(m.msg)
    {
	dbus_message_ref(msg);
    }


    Message::~Message()
    {
	dbus_message_unref(msg);
    }


    Message&
    Message::operator=(const Message& m)
    {
	if (this != &m)
	{
	    dbus_message_unref(msg);
	    msg = m.msg;
	    dbus_message_ref(msg);
	}
	return *this;
    }


    const char* TypeInfo<dbus_uint32_t>::signature = "u";
    const char* TypeInfo<string>::signature = "s";


    Hihi::Hihi(Message& msg)
    {
	DBusMessageIter args;
	if (!dbus_message_iter_init(msg.get_message(), &args))
	{
	    throw FatalException();
	}
	iters.push_back(args);
    }


    Hihi::~Hihi()
    {
	iters.pop_back();
	assert(iters.empty());
    }


    void
    Hihi::open_recurse()
    {
	DBusMessageIter iter2;
	dbus_message_iter_recurse(top(), &iter2);
	iters.push_back(iter2);
    }


    void
    Hihi::close_recurse()
    {
	iters.pop_back();
	dbus_message_iter_next(top());
    }


    Hoho::Hoho(Message& msg)
    {
	DBusMessageIter args;
	dbus_message_iter_init_append(msg.get_message(), &args);
	iters.push_back(args);
    }


    Hoho::~Hoho()
    {
	iters.pop_back();
	assert(iters.empty());
    }

    void
    Hoho::open_struct()
    {
	DBusMessageIter iter2;
	dbus_message_iter_open_container(top(), DBUS_TYPE_STRUCT, NULL, &iter2);
	iters.push_back(iter2);
    }


    void
    Hoho::close_struct()
    {
	DBusMessageIter iter2 = *top();
	iters.pop_back();
	dbus_message_iter_close_container(top(), &iter2);
    }


    void
    Hoho::open_array(const char* signature)
    {
	DBusMessageIter iter2;
	dbus_message_iter_open_container(top(), DBUS_TYPE_ARRAY, signature, &iter2);
	iters.push_back(iter2);
    }


    void
    Hoho::close_array()
    {
	DBusMessageIter iter2 = *top();
	iters.pop_back();
	dbus_message_iter_close_container(top(), &iter2);
    }


    void
    Hoho::open_dict_entry()
    {
	DBusMessageIter iter2;
	dbus_message_iter_open_container(top(), DBUS_TYPE_DICT_ENTRY, 0, &iter2);
	iters.push_back(iter2);
    }


    void
    Hoho::close_dict_entry()
    {
	DBusMessageIter iter2 = *top();
	iters.pop_back();
	dbus_message_iter_close_container(top(), &iter2);
    }


    Hihi&
    operator>>(Hihi& hihi, bool& data)
    {
	if (hihi.get_type() != DBUS_TYPE_BOOLEAN)
	    throw MarshallingException();

	dbus_bool_t tmp;
	dbus_message_iter_get_basic(hihi.top(), &tmp);
	dbus_message_iter_next(hihi.top());
	data = tmp;

	return hihi;
    }


    Hoho&
    operator<<(Hoho& hoho, bool data)
    {
	dbus_bool_t tmp = data;
	dbus_message_iter_append_basic(hoho.top(), DBUS_TYPE_BOOLEAN, &tmp);

	return hoho;
    }


    Hihi&
    operator>>(Hihi& hihi, dbus_uint16_t& data)
    {
	if (hihi.get_type() != DBUS_TYPE_UINT16)
	    throw MarshallingException();

	dbus_message_iter_get_basic(hihi.top(), &data);
	dbus_message_iter_next(hihi.top());

	return hihi;
    }


    Hoho&
    operator<<(Hoho& hoho, dbus_uint16_t data)
    {
	dbus_message_iter_append_basic(hoho.top(), DBUS_TYPE_UINT16, &data);

	return hoho;
    }


    Hihi&
    operator>>(Hihi& hihi, dbus_uint32_t& data)
    {
	if (hihi.get_type() != DBUS_TYPE_UINT32)
	    throw MarshallingException();

	dbus_message_iter_get_basic(hihi.top(), &data);
	dbus_message_iter_next(hihi.top());

	return hihi;
    }


    Hoho&
    operator<<(Hoho& hoho, dbus_uint32_t data)
    {
	dbus_message_iter_append_basic(hoho.top(), DBUS_TYPE_UINT32, &data);

	return hoho;
    }


    Hihi&
    operator>>(Hihi& hihi, time_t& data)
    {
	if (hihi.get_type() != DBUS_TYPE_UINT32)
	    throw MarshallingException();

	dbus_message_iter_get_basic(hihi.top(), &data);
	dbus_message_iter_next(hihi.top());

	return hihi;
    }


    Hoho&
    operator<<(Hoho& hoho, time_t data)
    {
	dbus_message_iter_append_basic(hoho.top(), DBUS_TYPE_UINT32, &data);

	return hoho;
    }


    Hoho&
    operator<<(Hoho& hoho, const char* data)
    {
	dbus_message_iter_append_basic(hoho.top(), DBUS_TYPE_STRING, &data);

	return hoho;
    }


    string
    Hihi::unescape(const string& in)
    {
	string out;

	for (string::const_iterator it = in.begin(); it != in.end(); ++it)
	{
	    if (*it == '\\')
	    {
		if (++it == in.end())
		    throw MarshallingException();

		if (*it == '\\')
		{
		    out += '\\';
		}
		else if (*it == 'x')
		{
		    if (++it == in.end())
			throw MarshallingException();

		    string t1;

		    if (!isxdigit(*it))
			throw MarshallingException();
		    t1 += *it;

		    if ((it + 1) != in.end() && isxdigit(*(it + 1)))
			t1 += *++it;

		    unsigned int t2;
		    sscanf(t1.c_str(), "%x", &t2);
		    out.append(1, (unsigned char)(t2));
		}
		else
		{
		    throw MarshallingException();
		}
	    }
	    else
	    {
		out += *it;
	    }
	}

	return out;
    }


    Hihi&
    operator>>(Hihi& hihi, string& data)
    {
	if (hihi.get_type() != DBUS_TYPE_STRING)
	    throw MarshallingException();

	const char* p = NULL;
	dbus_message_iter_get_basic(hihi.top(), &p);
	dbus_message_iter_next(hihi.top());
	data = hihi.unescape(p);

	return hihi;
    }


    string
    Hoho::escape(const string& in)
    {
	string out;

	for (string::const_iterator it = in.begin(); it != in.end(); ++it)
	{
	    if (*it == '\\')
	    {
		out += "\\\\";
	    }
	    else if ((unsigned char)(*it) > 127)
	    {
		char s[5];
		snprintf(s, 5, "\\x%x", (unsigned char)(*it));
		out += string(s);
	    }
	    else
	    {
		out += *it;
	    }
	}

	return out;
    }


    Hoho&
    operator<<(Hoho& hoho, const string& data)
    {
	string tmp = hoho.escape(data);
	const char* p = tmp.c_str();
	dbus_message_iter_append_basic(hoho.top(), DBUS_TYPE_STRING, &p);

	return hoho;
    }


    Hihi&
    operator>>(Hihi& hihi, map<string, string>& data)
    {
	if (hihi.get_type() != DBUS_TYPE_ARRAY)
	    throw MarshallingException();

	hihi.open_recurse();

	while (hihi.get_type() != DBUS_TYPE_INVALID)
	{
	    if (hihi.get_signature() != "{ss}")
		throw MarshallingException();

	    hihi.open_recurse();

	    string k, v;
	    hihi >> k >> v;
	    data[k] = v;

	    hihi.close_recurse();
	}

	hihi.close_recurse();

	return hihi;
    }


    Hoho&
    operator<<(Hoho& hoho, const map<string, string>& data)
    {
	hoho.open_array("{ss}");

	for (map<string, string>::const_iterator it = data.begin(); it != data.end() ; ++it)
	{
	    hoho.open_dict_entry();

	    hoho << it->first << it->second;

	    hoho.close_dict_entry();
	}

	hoho.close_array();

	return hoho;
    }

}
