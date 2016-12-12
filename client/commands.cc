/*
 * Copyright (c) [2012-2015] Novell, Inc.
 * Copyright (c) 2016 SUSE LLC
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


#include <iostream>

#include "commands.h"
#include "utils/text.h"
#include "snapper/AppUtil.h"

using namespace std;


#define SERVICE "org.opensuse.Snapper"
#define OBJECT "/org/opensuse/Snapper"
#define INTERFACE "org.opensuse.Snapper"


list<XConfigInfo>
command_list_xconfigs(DBus::Connection& conn)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "ListConfigs");

    DBus::Message reply = conn.send_with_reply_and_block(call);

    list<XConfigInfo> ret;

    DBus::Hihi hihi(reply);
    hihi >> ret;

    return ret;
}


XConfigInfo
command_get_xconfig(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "GetConfig");

    DBus::Hoho hoho(call);
    hoho << config_name;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    XConfigInfo ret;

    DBus::Hihi hihi(reply);
    hihi >> ret;

    return ret;
}


XSnapshots
command_list_xsnapshots(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "ListSnapshots");

    DBus::Hoho hoho(call);
    hoho << config_name;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    XSnapshots ret;

    DBus::Hihi hihi(reply);
    hihi >> ret.entries;

    return ret;
}


XSnapshot
command_get_xsnapshot(DBus::Connection& conn, const string& config_name, unsigned int num)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "GetSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << num;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    XSnapshot ret;

    DBus::Hihi hihi(reply);
    hihi >> ret;

    return ret;
}


void
command_set_xsnapshot(DBus::Connection& conn, const string& config_name, unsigned int num,
		      const XSnapshot& data)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "SetSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << num << data.description << data.cleanup << data.userdata;

    conn.send_with_reply_and_block(call);
}


unsigned int
command_create_single_xsnapshot(DBus::Connection& conn, const string& config_name,
				const string& description, const string& cleanup,
				const map<string, string>& userdata)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreateSingleSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << description << cleanup << userdata;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    unsigned int number;

    DBus::Hihi hihi(reply);
    hihi >> number;

    return number;
}


unsigned int
command_create_single_xsnapshot_v2(DBus::Connection& conn, const string& config_name,
				   unsigned int parent_num, bool read_only,
				   const string& description, const string& cleanup,
				   const map<string, string>& userdata)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreateSingleSnapshotV2");

    DBus::Hoho hoho(call);
    hoho << config_name << parent_num << read_only << description << cleanup << userdata;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    unsigned int number;

    DBus::Hihi hihi(reply);
    hihi >> number;

    return number;
}


unsigned int
command_create_single_xsnapshot_of_default(DBus::Connection& conn, const string& config_name,
					   bool read_only, const string& description,
					   const string& cleanup,
					   const map<string, string>& userdata)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreateSingleSnapshotOfDefault");

    DBus::Hoho hoho(call);
    hoho << config_name << read_only << description << cleanup << userdata;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    unsigned int number;

    DBus::Hihi hihi(reply);
    hihi >> number;

    return number;
}


void
command_delete_xsnapshots(DBus::Connection& conn, const string& config_name,
			  const list<unsigned int>& nums, bool verbose)
{
    if (verbose)
    {
	cout << sformat(_("Deleting snapshot from %s:", "Deleting snapshots from %s:", nums.size()),
			config_name.c_str()) << endl;

	for (list<unsigned int>::const_iterator it = nums.begin(); it != nums.end(); ++it)
	{
	    if (it != nums.begin())
		cout << ", ";
	    cout << *it;
	}
	cout << endl;
    }

    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "DeleteSnapshots");

    DBus::Hoho hoho(call);
    hoho << config_name << nums;

    conn.send_with_reply_and_block(call);
}


string
command_mount_xsnapshots(DBus::Connection& conn, const string& config_name,
			 unsigned int num, bool user_request)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "MountSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << num << user_request;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    string mount_point;

    DBus::Hihi hihi(reply);
    hihi >> mount_point;

    return mount_point;
}


string
command_get_xmount_point(DBus::Connection& conn, const string& config_name,
			 unsigned int num)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "GetMountPoint");

    DBus::Hoho hoho(call);
    hoho << config_name << num;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    string mount_point;

    DBus::Hihi hihi(reply);
    hihi >> mount_point;

    return mount_point;
}


int
operator<(const XFile& lhs, const XFile& rhs)
{
    return File::cmp_lt(lhs.name, rhs.name);
}


list<XFile>
command_get_xfiles(DBus::Connection& conn, const string& config_name, unsigned int number1,
		   unsigned int number2)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "GetFiles");

    DBus::Hoho hoho(call);
    hoho << config_name << number1 << number2;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    list<XFile> files;

    DBus::Hihi hihi(reply);
    hihi >> files;

    files.sort();		// snapperd can have different locale than client
				// so sorting is required here

    return files;
}


QuotaData
command_query_quota(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "QueryQuota");

    DBus::Hoho hoho(call);
    hoho << config_name;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    QuotaData quota_data;

    DBus::Hihi hihi(reply);
    hihi >> quota_data;

    return quota_data;
}
