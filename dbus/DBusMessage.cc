/*
 * Copyright (c) 2012 Novell, Inc.
 * Copyright (c) [2016-2025] SUSE LLC
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


#include <cassert>
#include <cstdlib>

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


    const char* TypeInfo<dbus_int32_t>::signature = "i";
    const char* TypeInfo<dbus_uint32_t>::signature = "u";
    const char* TypeInfo<dbus_uint64_t>::signature = "t";
    const char* TypeInfo<string>::signature = "s";


    string
    Marshalling::get_signature()
    {
	char* tmp = dbus_message_iter_get_signature(top());
	if (!tmp)
	    SN_THROW(FatalException());

	string sig = tmp;
	dbus_free(tmp);
	return sig;
    }


    Unmarshaller::Unmarshaller(Message& msg)
    {
	iters.push_back(new DBusMessageIter());
	if (!dbus_message_iter_init(msg.get_message(), top()))
	    SN_THROW(FatalException());
    }


    Unmarshaller::~Unmarshaller()
    {
	delete iters.back();
	iters.pop_back();
	assert(iters.empty());
    }


    void
    Unmarshaller::open_recurse()
    {
	DBusMessageIter* iter2 = new DBusMessageIter();
	dbus_message_iter_recurse(top(), iter2);
	iters.push_back(iter2);
    }


    void
    Unmarshaller::close_recurse()
    {
	delete iters.back();
	iters.pop_back();
	dbus_message_iter_next(top());
    }


    Marshaller::Marshaller(Message& msg)
    {
	iters.push_back(new DBusMessageIter());
	dbus_message_iter_init_append(msg.get_message(), top());
    }


    Marshaller::~Marshaller()
    {
	delete iters.back();
	iters.pop_back();
	assert(iters.empty());
    }


    void
    Marshaller::open_struct()
    {
	DBusMessageIter* iter2 = new DBusMessageIter();
	if (!dbus_message_iter_open_container(top(), DBUS_TYPE_STRUCT, NULL, iter2))
	    SN_THROW(FatalException());
	iters.push_back(iter2);
    }


    void
    Marshaller::close_struct()
    {
	DBusMessageIter* iter2 = top();
	iters.pop_back();
	if (!dbus_message_iter_close_container(top(), iter2))
	    SN_THROW(FatalException());
	delete iter2;
    }


    void
    Marshaller::open_array(const char* signature)
    {
	DBusMessageIter* iter2 = new DBusMessageIter();
	if (!dbus_message_iter_open_container(top(), DBUS_TYPE_ARRAY, signature, iter2))
	    SN_THROW(FatalException());
	iters.push_back(iter2);
    }


    void
    Marshaller::close_array()
    {
	DBusMessageIter* iter2 = top();
	iters.pop_back();
	if (!dbus_message_iter_close_container(top(), iter2))
	    SN_THROW(FatalException());
	delete iter2;
    }


    void
    Marshaller::open_dict_entry()
    {
	DBusMessageIter* iter2 = new DBusMessageIter();
	if (!dbus_message_iter_open_container(top(), DBUS_TYPE_DICT_ENTRY, 0, iter2))
	    SN_THROW(FatalException());
	iters.push_back(iter2);
    }


    void
    Marshaller::close_dict_entry()
    {
	DBusMessageIter* iter2 = top();
	iters.pop_back();
	if (!dbus_message_iter_close_container(top(), iter2))
	    SN_THROW(FatalException());
	delete iter2;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, bool& data)
    {
	if (unmarshaller.get_type() != DBUS_TYPE_BOOLEAN)
	    SN_THROW(MarshallingException());

	dbus_bool_t tmp;
	dbus_message_iter_get_basic(unmarshaller.top(), &tmp);
	dbus_message_iter_next(unmarshaller.top());
	data = tmp;

	return unmarshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, bool data)
    {
	dbus_bool_t tmp = data;
	if (!dbus_message_iter_append_basic(marshaller.top(), DBUS_TYPE_BOOLEAN, &tmp))
	    SN_THROW(FatalException());

	return marshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, dbus_uint16_t& data)
    {
	if (unmarshaller.get_type() != DBUS_TYPE_UINT16)
	    SN_THROW(MarshallingException());

	dbus_message_iter_get_basic(unmarshaller.top(), &data);
	dbus_message_iter_next(unmarshaller.top());

	return unmarshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, dbus_uint16_t data)
    {
	if (!dbus_message_iter_append_basic(marshaller.top(), DBUS_TYPE_UINT16, &data))
	    SN_THROW(FatalException());

	return marshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, dbus_int32_t& data)
    {
	if (unmarshaller.get_type() != DBUS_TYPE_INT32)
	    SN_THROW(MarshallingException());

	dbus_message_iter_get_basic(unmarshaller.top(), &data);
	dbus_message_iter_next(unmarshaller.top());

	return unmarshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, dbus_int32_t data)
    {
	if (!dbus_message_iter_append_basic(marshaller.top(), DBUS_TYPE_INT32, &data))
	    SN_THROW(FatalException());

	return marshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, dbus_uint32_t& data)
    {
	if (unmarshaller.get_type() != DBUS_TYPE_UINT32)
	    SN_THROW(MarshallingException());

	dbus_message_iter_get_basic(unmarshaller.top(), &data);
	dbus_message_iter_next(unmarshaller.top());

	return unmarshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, dbus_uint32_t data)
    {
	if (!dbus_message_iter_append_basic(marshaller.top(), DBUS_TYPE_UINT32, &data))
	    SN_THROW(FatalException());

	return marshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, dbus_uint64_t& data)
    {
	if (unmarshaller.get_type() != DBUS_TYPE_UINT64)
	    SN_THROW(MarshallingException());

	dbus_message_iter_get_basic(unmarshaller.top(), &data);
	dbus_message_iter_next(unmarshaller.top());

	return unmarshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, dbus_uint64_t data)
    {
	if (!dbus_message_iter_append_basic(marshaller.top(), DBUS_TYPE_UINT64, &data))
	    SN_THROW(FatalException());

	return marshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, time_t& data)
    {
	if (unmarshaller.get_type() != DBUS_TYPE_INT64)
	    SN_THROW(MarshallingException());

	dbus_uint64_t tmp;
	dbus_message_iter_get_basic(unmarshaller.top(), &tmp);
	dbus_message_iter_next(unmarshaller.top());
	data = tmp;

	return unmarshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, time_t data)
    {
	dbus_uint64_t tmp = data;
	if (!dbus_message_iter_append_basic(marshaller.top(), DBUS_TYPE_INT64, &tmp))
	    SN_THROW(FatalException());

	return marshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const char* data)
    {
	if (!dbus_message_iter_append_basic(marshaller.top(), DBUS_TYPE_STRING, &data))
	    SN_THROW(FatalException());

	return marshaller;
    }


    string
    Unmarshaller::unescape(const string& in)
    {
	string out;

	for (string::const_iterator it = in.begin(); it != in.end(); ++it)
	{
	    if (*it == '\\')
	    {
		if (++it == in.end())
		    SN_THROW(MarshallingException());

		if (*it == '\\')
		{
		    out += '\\';
		}
		else if (*it == 'x')
		{
		    string t1;
		    for (int i = 0; i < 2; ++i)
		    {
			if (++it == in.end() || !isxdigit(*it))
			    SN_THROW(MarshallingException());
			t1 += *it;
		    }

		    unsigned int t2;
		    sscanf(t1.c_str(), "%x", &t2);
		    out.append(1, (unsigned char)(t2));
		}
		else
		{
		    SN_THROW(MarshallingException());
		}
	    }
	    else
	    {
		out += *it;
	    }
	}

	return out;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, string& data)
    {
	if (unmarshaller.get_type() != DBUS_TYPE_STRING)
	    SN_THROW(MarshallingException());

	const char* p = NULL;
	dbus_message_iter_get_basic(unmarshaller.top(), &p);
	dbus_message_iter_next(unmarshaller.top());
	data = unmarshaller.unescape(p);

	return unmarshaller;
    }


    string
    Marshaller::escape(const string& in)
    {
	string out;

	for (const char c : in)
	{
	    if (c == '\\')
	    {
		out += "\\\\";
	    }
	    else if ((unsigned char)(c) > 127)
	    {
		char s[5];
		snprintf(s, 5, "\\x%02x", (unsigned char)(c));
		out += string(s);
	    }
	    else
	    {
		out += c;
	    }
	}

	return out;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const string& data)
    {
	string tmp = marshaller.escape(data);
	const char* p = tmp.c_str();
	if (!dbus_message_iter_append_basic(marshaller.top(), DBUS_TYPE_STRING, &p))
	    SN_THROW(FatalException());

	return marshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, map<string, string>& data)
    {
	if (unmarshaller.get_type() != DBUS_TYPE_ARRAY)
	    SN_THROW(MarshallingException());

	unmarshaller.open_recurse();

	while (unmarshaller.get_type() != DBUS_TYPE_INVALID)
	{
	    if (unmarshaller.get_signature() != "{ss}")
		SN_THROW(MarshallingException());

	    unmarshaller.open_recurse();

	    string k, v;
	    unmarshaller >> k >> v;
	    data[k] = v;

	    unmarshaller.close_recurse();
	}

	unmarshaller.close_recurse();

	return unmarshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const map<string, string>& data)
    {
	marshaller.open_array("{ss}");

	for (map<string, string>::const_iterator it = data.begin(); it != data.end() ; ++it)
	{
	    marshaller.open_dict_entry();

	    marshaller << it->first << it->second;

	    marshaller.close_dict_entry();
	}

	marshaller.close_array();

	return marshaller;
    }

}
