/*
 * Copyright (c) [2012-2015] Novell, Inc.
 * Copyright (c) [2016,2018] SUSE LLC
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


#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

#include "types.h"


vector<XConfigInfo>
command_list_xconfigs(DBus::Connection& conn);

XConfigInfo
command_get_xconfig(DBus::Connection& conn, const string& config_name);

void
command_set_xconfig(DBus::Connection& conn, const string& config_name,
		    const map<string, string>& raw);

void
command_create_config(DBus::Connection& conn, const string& config_name, const string& subvolume,
		      const string& fstype, const string& template_name);

void
command_delete_config(DBus::Connection& conn, const string& config_name);

XSnapshots
command_list_xsnapshots(DBus::Connection& conn, const string& config_name);

XSnapshot
command_get_xsnapshot(DBus::Connection& conn, const string& config_name, unsigned int num);

void
command_set_snapshot(DBus::Connection& conn, const string& config_name, unsigned int num,
		     const SMD& smd);

unsigned int
command_create_single_snapshot(DBus::Connection& conn, const string& config_name,
			       const string& description, const string& cleanup,
			       const map<string, string>& userdata);

unsigned int
command_create_single_snapshot_v2(DBus::Connection& conn, const string& config_name,
				  unsigned int parent_num, bool read_only,
				  const string& description, const string& cleanup,
				  const map<string, string>& userdata);

unsigned int
command_create_single_snapshot_of_default(DBus::Connection& conn, const string& config_name,
					  bool read_only, const string& description,
					  const string& cleanup,
					  const map<string, string>& userdata);

unsigned int
command_create_pre_snapshot(DBus::Connection& conn, const string& config_name,
			    const string& description, const string& cleanup,
			    const map<string, string>& userdata);

unsigned int
command_create_post_snapshot(DBus::Connection& conn, const string& config_name,
			     unsigned int prenum, const string& description,
			     const string& cleanup, const map<string, string>& userdata);

void
command_delete_snapshots(DBus::Connection& conn, const string& config_name,
			 const vector<unsigned int>& nums, bool verbose);

void
command_calculate_used_space(DBus::Connection& conn, const string& config_name);

uint64_t
command_get_used_space(DBus::Connection& conn, const string& config_name, unsigned int num);

string
command_mount_snapshot(DBus::Connection& conn, const string& config_name,
		       unsigned int num, bool user_request);

void
command_umount_snapshot(DBus::Connection& conn, const string& config_name,
			unsigned int num, bool user_request);

string
command_get_mount_point(DBus::Connection& conn, const string& config_name,
			unsigned int num);

void
command_create_comparison(DBus::Connection& conn, const string& config_name, unsigned int number1,
			  unsigned int number2);

void
command_delete_comparison(DBus::Connection& conn, const string& config_name, unsigned int number1,
			  unsigned int number2);

vector<XFile>
command_get_xfiles(DBus::Connection& conn, const string& config_name, unsigned int number1,
		   unsigned int number2);

void
command_setup_quota(DBus::Connection& conn, const string& config_name);

void
command_prepare_quota(DBus::Connection& conn, const string& config_name);

QuotaData
command_query_quota(DBus::Connection& conn, const string& config_name);

void
command_sync(DBus::Connection& conn, const string& config_name);

vector<string>
command_debug(DBus::Connection& conn);
