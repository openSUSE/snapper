/*
 * Copyright (c) [2012-2015] Novell, Inc.
 * Copyright (c) [2016-2022] SUSE LLC
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


#include <stdio.h>
#include <iostream>

#include "commands.h"
#include "errors.h"
#include "utils/text.h"
#include "dbus/DBusPipe.h"
#include "snapper/AppUtil.h"

using namespace std;


#define SERVICE "org.opensuse.Snapper"
#define OBJECT "/org/opensuse/Snapper"
#define INTERFACE "org.opensuse.Snapper"


vector<XConfigInfo>
command_list_xconfigs(DBus::Connection& conn)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "ListConfigs");

    DBus::Message reply = conn.send_with_reply_and_block(call);

    vector<XConfigInfo> ret;

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


void
command_set_xconfig(DBus::Connection& conn, const string& config_name,
		    const map<string, string>& raw)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "SetConfig");

    DBus::Hoho hoho(call);
    hoho << config_name << raw;

    conn.send_with_reply_and_block(call);
}


void
command_create_config(DBus::Connection& conn, const string& config_name, const string& subvolume,
		      const string& fstype, const string& template_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreateConfig");

    DBus::Hoho hoho(call);
    hoho << config_name << subvolume << fstype << template_name;

    conn.send_with_reply_and_block(call);
}


void
command_delete_config(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "DeleteConfig");

    DBus::Hoho hoho(call);
    hoho << config_name;

    conn.send_with_reply_and_block(call);
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
command_set_snapshot(DBus::Connection& conn, const string& config_name, unsigned int num,
		     const SMD& smd)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "SetSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << num << smd.description << smd.cleanup << smd.userdata;

    conn.send_with_reply_and_block(call);
}


unsigned int
command_create_single_snapshot(DBus::Connection& conn, const string& config_name,
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
command_create_single_snapshot_v2(DBus::Connection& conn, const string& config_name,
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
command_create_single_snapshot_of_default(DBus::Connection& conn, const string& config_name,
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


unsigned int
command_create_pre_snapshot(DBus::Connection& conn, const string& config_name,
			    const string& description, const string& cleanup,
			    const map<string, string>& userdata)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreatePreSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << description << cleanup << userdata;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    unsigned int number;

    DBus::Hihi hihi(reply);
    hihi >> number;

    return number;
}


unsigned int
command_create_post_snapshot(DBus::Connection& conn, const string& config_name,
			     unsigned int prenum, const string& description,
			     const string& cleanup, const map<string, string>& userdata)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreatePostSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << prenum << description << cleanup << userdata;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    unsigned int number;

    DBus::Hihi hihi(reply);
    hihi >> number;

    return number;
}


void
command_delete_snapshots(DBus::Connection& conn, const string& config_name,
			 const vector<unsigned int>& nums, bool verbose)
{
    if (verbose)
    {
	cout << sformat(_("Deleting snapshot from %s:", "Deleting snapshots from %s:", nums.size()),
			config_name.c_str()) << endl;

	for (vector<unsigned int>::const_iterator it = nums.begin(); it != nums.end(); ++it)
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


std::pair<bool, unsigned int>
command_get_default_snapshot(DBus::Connection& conn, const string& config_name)
{
    try
    {
	DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "GetDefaultSnapshot");

	DBus::Hoho hoho(call);
	hoho << config_name;

	DBus::Message reply = conn.send_with_reply_and_block(call);

	bool valid;
	unsigned int number;

	DBus::Hihi hihi(reply);
	hihi >> valid >> number;

	return make_pair(valid, number);
    }
    catch (const DBus::ErrorException& e)
    {
	convert_exception(e);
    }
}


std::pair<bool, unsigned int>
command_get_active_snapshot(DBus::Connection& conn, const string& config_name)
{
    try
    {
	DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "GetActiveSnapshot");

	DBus::Hoho hoho(call);
	hoho << config_name;

	DBus::Message reply = conn.send_with_reply_and_block(call);

	bool valid;
	unsigned int number;

	DBus::Hihi hihi(reply);
	hihi >> valid >> number;

	return make_pair(valid, number);
    }
    catch (const DBus::ErrorException& e)
    {
	convert_exception(e);
    }
}


void
command_calculate_used_space(DBus::Connection& conn, const string& config_name)
{
    try
    {
	DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CalculateUsedSpace");

	DBus::Hoho hoho(call);
	hoho << config_name;

	conn.send_with_reply_and_block(call);
    }
    catch (const DBus::ErrorException& e)
    {
	convert_exception(e);
    }
}


uint64_t
command_get_used_space(DBus::Connection& conn, const string& config_name, unsigned int num)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "GetUsedSpace");

    DBus::Hoho hoho(call);
    hoho << config_name << num;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    uint64_t used_space;

    DBus::Hihi hihi(reply);
    hihi >> used_space;

    return used_space;
}


string
command_mount_snapshot(DBus::Connection& conn, const string& config_name,
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


void
command_umount_snapshot(DBus::Connection& conn, const string& config_name, unsigned int num,
			bool user_request)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "UmountSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << num << user_request;

    conn.send_with_reply_and_block(call);
}


string
command_get_mount_point(DBus::Connection& conn, const string& config_name, unsigned int num)
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


void
command_create_comparison(DBus::Connection& conn, const string& config_name, unsigned int number1,
			  unsigned int number2)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreateComparison");

    DBus::Hoho hoho(call);
    hoho << config_name << number1 << number2;

    conn.send_with_reply_and_block(call);
}


void
command_delete_comparison(DBus::Connection& conn, const string& config_name, unsigned int number1,
			  unsigned int number2)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "DeleteComparison");

    DBus::Hoho hoho(call);
    hoho << config_name << number1 << number2;

    conn.send_with_reply_and_block(call);
}


int
operator<(const XFile& lhs, const XFile& rhs)
{
    return File::cmp_lt(lhs.name, rhs.name);
}


vector<XFile>
command_get_xfiles(DBus::Connection& conn, const string& config_name, unsigned int number1,
		   unsigned int number2)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "GetFiles");

    DBus::Hoho hoho(call);
    hoho << config_name << number1 << number2;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    vector<XFile> files;

    DBus::Hihi hihi(reply);
    hihi >> files;

    sort(files.begin(), files.end());

    return files;
}


vector<XFile>
command_get_xfiles_by_pipe(DBus::Connection& conn, const string& config_name, unsigned int number1,
			   unsigned int number2)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "GetFilesByPipe");

    DBus::Hoho hoho(call);
    hoho << config_name << number1 << number2;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    DBus::FileDescriptor fd;

    DBus::Hihi hihi(reply);
    hihi >> fd;

    vector<XFile> files;

    FILE* fin = fdopen(fd.get_fd(), "r");
    if (!fin)
	SN_THROW(IOErrorException("reading pipe failed, fdopen failed: " + stringerror(errno)));

    char* buffer = nullptr;
    size_t len = 0;

    while (true)
    {
	ssize_t n = getline(&buffer, &len, fin);
	if (n == -1)
	{
	    if (feof(fin) != 0)
		break;

	    SN_THROW(IOErrorException("reading pipe failed, getline failed: " + stringerror(errno)));
	}

	if (n > 0 && buffer[n - 1] == '\n')
	    --n;

	const string line = string(buffer, 0, n);

	string::size_type pos1 = line.find(" ");
	if (pos1 == string::npos)
	    SN_THROW(IOErrorException("reading pipe failed, parse error"));

	string::size_type pos2 = line.find(" ", pos1 + 1);

	XFile file;
	file.name = DBus::Pipe::unescape(line.substr(0, pos1));
	file.status = stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
	files.push_back(file);
    }

    free(buffer);

    if (fclose(fin) != 0)
	SN_THROW(IOErrorException("reading pipe failed, fclose failed: " + stringerror(errno)));

    sort(files.begin(), files.end());

    return files;
}


void
command_setup_quota(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "SetupQuota");

    DBus::Hoho hoho(call);
    hoho << config_name;

    conn.send_with_reply_and_block(call);
}


void
command_prepare_quota(DBus::Connection& conn, const string& config_name)
{
    try
    {
	DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "PrepareQuota");

	DBus::Hoho hoho(call);
	hoho << config_name;

	conn.send_with_reply_and_block(call);
    }
    catch (const DBus::ErrorException& e)
    {
	convert_exception(e);
    }
}


QuotaData
command_query_quota(DBus::Connection& conn, const string& config_name)
{
    try
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
    catch (const DBus::ErrorException& e)
    {
	convert_exception(e);
    }
}


FreeSpaceData
command_query_free_space(DBus::Connection& conn, const string& config_name)
{
    try
    {
	DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "QueryFreeSpace");

	DBus::Hoho hoho(call);
	hoho << config_name;

	DBus::Message reply = conn.send_with_reply_and_block(call);

	FreeSpaceData free_space_data;

	DBus::Hihi hihi(reply);
	hihi >> free_space_data;

	return free_space_data;
    }
    catch (const DBus::ErrorException& e)
    {
	convert_exception(e);
    }
}


void
command_sync(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "Sync");

    DBus::Hoho hoho(call);
    hoho << config_name;

    conn.send_with_reply_and_block(call);
}


vector<string>
command_debug(DBus::Connection& conn)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "Debug");

    DBus::Message reply = conn.send_with_reply_and_block(call);

    vector<string> lines;

    DBus::Hihi hihi(reply);
    hihi >> lines;

    return lines;
}
