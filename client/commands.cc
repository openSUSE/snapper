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

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include "commands.h"
#include "sck/SyncReadStream.h"

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
command_create_xconfig(DBus::Connection& conn, const string& config_name, const string& subvolume,
		       const string& fstype, const string& template_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreateConfig");

    DBus::Hoho hoho(call);
    hoho << config_name << subvolume << fstype << template_name;

    conn.send_with_reply_and_block(call);
}


void
command_delete_xconfig(DBus::Connection& conn, const string& config_name)
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


unsigned int
command_create_pre_xsnapshot(DBus::Connection& conn, const string& config_name,
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
command_create_post_xsnapshot(DBus::Connection& conn, const string& config_name,
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
command_delete_xsnapshots(DBus::Connection& conn, const string& config_name,
			  list<unsigned int> nums)
{
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


void
command_umount_xsnapshots(DBus::Connection& conn, const string& config_name,
			  unsigned int num, bool user_request)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "UmountSnapshot");

    DBus::Hoho hoho(call);
    hoho << config_name << num << user_request;

    conn.send_with_reply_and_block(call);
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


void
command_create_xcomparison(DBus::Connection& conn, const string& config_name, unsigned int number1,
			   unsigned int number2)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "CreateComparison");

    DBus::Hoho hoho(call);
    hoho << config_name << number1 << number2;

    conn.send_with_reply_and_block(call);
}


void
command_delete_xcomparison(DBus::Connection& conn, const string& config_name, unsigned int number1,
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


list<XFile>
command_get_xfiles_socket(DBus::Connection& conn, const string& config_name, unsigned int number1,
			  unsigned int number2)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "GetFilesBySocket");

    DBus::Hoho hoho(call);
    hoho << config_name << "/" << number1 << number2;

    DBus::Message reply = conn.send_with_reply_and_block(call);

    sck::SocketFd fd;
    DBus::Hihi hihi(reply);
    hihi >> fd;

    sck::ReadStream<vector<XFile *>> rs(fd);

    list<XFile> files;

    struct Ffunct {
	Ffunct(list<XFile>& fs) : files(fs) {}
	void operator()(XFile* f) { files.push_back(*f); delete f; }
	list<XFile>& files;
    };

    Ffunct ff(files);

    while (rs.incoming())
    {
	vector<XFile *> tmp = rs.receive();
	std::for_each(tmp.begin(), tmp.end(), ff);
    }

    files.sort();		// snapperd can have different locale than client
				// so sorting is required here

    return files;
}


void
command_xsync(DBus::Connection& conn, const string& config_name)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "Sync");

    DBus::Hoho hoho(call);
    hoho << config_name;

    conn.send_with_reply_and_block(call);
}


vector<string>
command_xdebug(DBus::Connection& conn)
{
    DBus::MessageMethodCall call(SERVICE, OBJECT, INTERFACE, "Debug");

    DBus::Message reply = conn.send_with_reply_and_block(call);

    vector<string> lines;

    DBus::Hihi hihi(reply);
    hihi >> lines;

    return lines;
}
