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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#include "commands.h"


list<XConfigInfo>
command_list_xconfigs(DBus::Connection& conn)
{
    DBus::MessageMethodCall call("org.opensuse.snapper", "/org/opensuse/snapper",
				 "org.opensuse.snapper", "ListConfigs");

    DBus::Message reply = conn.send_and_reply_and_block(call);

    list<XConfigInfo> ret;

    DBus::Hihi hihi(reply);
    hihi >> ret;

    return ret;
}


list<XSnapshot>
command_list_xsnapshots(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageMethodCall call("org.opensuse.snapper", "/org/opensuse/snapper",
				 "org.opensuse.snapper", "ListSnapshots");

    DBus::Hoho hoho(call);
    hoho << config_name;

    DBus::Message reply = conn.send_and_reply_and_block(call);

    list<XSnapshot> ret;

    DBus::Hihi hihi(reply);
    hihi >> ret;

    return ret;
}


unsigned int
command_create_single_xsnapshot(DBus::Connection& conn, const string& config_name,
				const string& description, const string& cleanup,
				const map<string, string>& userdata)
{
    DBus::MessageMethodCall call("org.opensuse.snapper", "/org/opensuse/snapper",
				 "org.opensuse.snapper", "CreateSingleSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << description << cleanup << userdata;

    DBus::Message reply = conn.send_and_reply_and_block(call);

    unsigned int number;

    DBus::Hihi hihi(reply);
    hihi >> number;

    return number;
}


unsigned int
command_create_pre_xsnapshot(DBus::Connection& conn, const string& config_name,
			     const string& description, const string& cleanup,
			     const map<string, string>& userdata)
{
    DBus::MessageMethodCall call("org.opensuse.snapper", "/org/opensuse/snapper",
				 "org.opensuse.snapper", "CreatePreSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << description << cleanup << userdata;

    DBus::Message reply = conn.send_and_reply_and_block(call);

    unsigned int number;

    DBus::Hihi hihi(reply);
    hihi >> number;

    return number;
}


unsigned int
command_create_post_xsnapshot(DBus::Connection& conn, const string& config_name,
			      unsigned int prenum, const string& description,
			      const string& cleanup, const map<string, string>& userdata)
{
    DBus::MessageMethodCall call("org.opensuse.snapper", "/org/opensuse/snapper",
				 "org.opensuse.snapper", "CreatePostSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << prenum << description << cleanup << userdata;

    DBus::Message reply = conn.send_and_reply_and_block(call);

    unsigned int number;

    DBus::Hihi hihi(reply);
    hihi >> number;

    return number;
}


void
command_create_xcomparison(DBus::Connection& conn, const string& config_name, unsigned int number1,
			   unsigned int number2)
{
    DBus::MessageMethodCall call("org.opensuse.snapper", "/org/opensuse/snapper",
                                 "org.opensuse.snapper", "CreateComparison");

    DBus::Hoho hoho(call);
    hoho << config_name << number1 << number2;

    DBus::Message reply = conn.send_and_reply_and_block(call);
}


list<XFile>
command_get_xfiles(DBus::Connection& conn, const string& config_name, unsigned int number1,
		   unsigned int number2)
{
    DBus::MessageMethodCall call("org.opensuse.snapper", "/org/opensuse/snapper",
                                 "org.opensuse.snapper", "GetFiles");

    DBus::Hoho hoho(call);
    hoho << config_name << number1 << number2;

    DBus::Message reply = conn.send_and_reply_and_block(call);

    DBus::Hihi hihi(reply);
    list<XFile> files;
    hihi >> files;

    return files;
}


vector<string>
command_get_xdiff(DBus::Connection& conn, const string& config_name, unsigned int number1,
		  unsigned int number2, const string& filename, const string& options)
{
    DBus::MessageMethodCall call("org.opensuse.snapper", "/org/opensuse/snapper",
                                 "org.opensuse.snapper", "GetDiff");

    DBus::Hoho hoho(call);
    hoho << config_name << number1 << number2 << filename << options;

    DBus::Message reply = conn.send_and_reply_and_block(call);

    DBus::Hihi hihi(reply);
    vector<string> files;
    hihi >> files;

    return files;
}
