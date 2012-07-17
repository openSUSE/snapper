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


#include <string>
#include <vector>
#include <list>
#include <map>

using std::string;
using std::vector;
using std::list;
using std::map;

#include "types.h"


list<XConfigInfo>
command_list_xconfigs(DBus::Connection& conn);

XConfigInfo
command_get_xconfig(DBus::Connection& conn, const string& config_name);

void
command_create_xconfig(DBus::Connection& conn, const string& config_name, const string& subvolume,
		       const string& fstype, const string& template_name);

void
command_delete_xconfig(DBus::Connection& conn, const string& config_name);

XSnapshots
command_list_xsnapshots(DBus::Connection& conn, const string& config_name);

XSnapshot
command_get_xsnapshot(DBus::Connection& conn, const string& config_name, unsigned int num);

void
command_set_xsnapshot(DBus::Connection& conn, const string& config_name, unsigned int num,
		      const XSnapshot& data);

unsigned int
command_create_single_xsnapshot(DBus::Connection& conn, const string& config_name,
				const string& description, const string& cleanup,
				const map<string, string>& userdata);

unsigned int
command_create_pre_xsnapshot(DBus::Connection& conn, const string& config_name,
			     const string& description, const string& cleanup,
			     const map<string, string>& userdata);

unsigned int
command_create_post_xsnapshot(DBus::Connection& conn, const string& config_name,
			      unsigned int prenum, const string& description,
			      const string& cleanup, const map<string, string>& userdata);

void
command_delete_xsnapshots(DBus::Connection& conn, const string& config_name,
			  list<unsigned int> nums);

void
command_mount_xsnapshots(DBus::Connection& conn, const string& config_name,
			 unsigned int num);

void
command_umount_xsnapshots(DBus::Connection& conn, const string& config_name,
			  unsigned int num);

void
command_create_xcomparison(DBus::Connection& conn, const string& config_name, unsigned int number1,
			   unsigned int number2);

list<XFile>
command_get_xfiles(DBus::Connection& conn, const string& config_name, unsigned int number1,
		   unsigned int number2);

vector<string>
command_get_xdiff(DBus::Connection& conn, const string& config_name, unsigned int number1,
		  unsigned int number2, const string& name, const string& options);

void
command_set_xundo(DBus::Connection& conn, const string& config_name, unsigned int number1,
		  unsigned int number2, const list<XUndo>& undos);

void
command_set_xundo_all(DBus::Connection& conn, const string& config_name, unsigned int number1,
		      unsigned int number2, bool undo);

vector<XUndoStep>
command_get_xundo_steps(DBus::Connection& conn, const string& config_name, unsigned int number1,
			unsigned int number2);

bool
command_do_xundo_step(DBus::Connection& conn, const string& config_name, unsigned int number1,
		      unsigned int number2, const XUndoStep& undo_step);

vector<string>
command_xdebug(DBus::Connection& conn);
