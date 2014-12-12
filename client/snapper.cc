/*
 * Copyright (c) [2011-2014] Novell, Inc.
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


#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream>
#include <boost/algorithm/string.hpp>

#include <snapper/Snapper.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/Enum.h>
#include <snapper/AsciiFile.h>
#include <snapper/SnapperDefines.h>
#include <snapper/XAttributes.h>
#ifdef ENABLE_ROLLBACK
#include <snapper/Filesystem.h>
#endif

#include "utils/text.h"
#include "utils/Table.h"
#include "utils/GetOpts.h"

#include "commands.h"
#include "cleanup.h"
#include "errors.h"
#include "misc.h"


using namespace snapper;
using namespace std;


struct Cmd
{
    typedef void (*cmd_func_t)(DBus::Connection* conn, Snapper* snapper);
    typedef void (*help_func_t)();

    Cmd(const string& name, cmd_func_t cmd_func, help_func_t help_func,
	bool works_without_dbus, bool needs_snapper)
	: name(name), cmd_func(cmd_func), help_func(help_func),
	  works_without_dbus(works_without_dbus), needs_snapper(needs_snapper)
    {}

    Cmd(const string& name, const vector<string>& aliases, cmd_func_t cmd_func,
	help_func_t help_func, bool works_without_dbus, bool needs_snapper)
	: name(name), aliases(aliases), cmd_func(cmd_func), help_func(help_func),
	  works_without_dbus(works_without_dbus), needs_snapper(needs_snapper)
    {}

    const string name;
    const vector<string> aliases;
    const cmd_func_t cmd_func;
    const help_func_t help_func;
    const bool works_without_dbus;
    const bool needs_snapper;
};

GetOpts getopts;

bool quiet = false;
bool verbose = false;
bool utc = false;
bool iso = false;
string config_name = "root";
bool no_dbus = false;


struct MyFiles : public Files
{
    friend class MyComparison;

    MyFiles(const FilePaths* file_paths)
	: Files(file_paths) {}
};


struct MyComparison
{
    MyComparison(DBus::Connection& conn, pair<unsigned int, unsigned int> nums, bool mount)
	: files(&file_paths)
    {
	command_create_xcomparison(conn, config_name, nums.first, nums.second);

	file_paths.system_path = command_get_xmount_point(conn, config_name, 0);

	if (mount)
	{
	    if (nums.first != 0)
		file_paths.pre_path = command_mount_xsnapshots(conn, config_name, nums.first, false);
	    else
		file_paths.pre_path = file_paths.system_path;

	    if (nums.second != 0)
		file_paths.post_path = command_mount_xsnapshots(conn, config_name, nums.second, false);
	    else
		file_paths.post_path = file_paths.system_path;
	}

	list<XFile> tmp = command_get_xfiles(conn, config_name, nums.first, nums.second);
	for (list<XFile>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
	    files.push_back(File(&file_paths, it->name, it->status));
    }

    FilePaths file_paths;

    MyFiles files;
};


void
help_list_configs()
{
    cout << _("  List configs:") << endl
	 << _("\tsnapper list-configs") << endl
	 << endl;
}


list<pair<string, string>>
enum_configs(DBus::Connection* conn)
{
    list<pair<string, string>> configs;

    if (no_dbus)
    {
	list<ConfigInfo> config_infos = Snapper::getConfigs();
	for (list<ConfigInfo>::const_iterator it = config_infos.begin(); it != config_infos.end(); ++it)
	{
	    configs.push_back(make_pair(it->getConfigName(), it->getSubvolume()));
	}
    }
    else
    {
	list<XConfigInfo> config_infos = command_list_xconfigs(*conn);
	for (list<XConfigInfo>::const_iterator it = config_infos.begin(); it != config_infos.end(); ++it)
	{
	    configs.push_back(make_pair(it->config_name, it->subvolume));
	}
    }

    return configs;
}


void
command_list_configs(DBus::Connection* conn, Snapper* snapper)
{
    getopts.parse("list-configs", GetOpts::no_options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'list-configs' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    Table table;

    TableHeader header;
    header.add(_("Config"));
    header.add(_("Subvolume"));
    table.setHeader(header);

    list<pair<string, string> > configs = enum_configs(conn);

    for (list<pair<string,string> >::iterator it = configs.begin(); it != configs.end(); ++it)
    {
	TableRow row;
	row.add(it->first);
	row.add(it->second);
	table.add(row);
    }

    cout << table;
}


void
help_create_config()
{
    cout << _("  Create config:") << endl
	 << _("\tsnapper create-config <subvolume>") << endl
	 << endl
	 << _("    Options for 'create-config' command:") << endl
	 << _("\t--fstype, -f <fstype>\t\tManually set filesystem type.") << endl
	 << _("\t--template, -t <name>\t\tName of config template to use.") << endl
	 << endl;
}


void
command_create_config(DBus::Connection* conn, Snapper* snapper)
{
    const struct option options[] = {
	{ "fstype",		required_argument,	0,	'f' },
	{ "template",		required_argument,	0,	't' },
#ifdef ENABLE_ROLLBACK
	{ "add-fstab",		no_argument,		0,	0 },
#endif
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("create-config", options);
    if (getopts.numArgs() != 1)
    {
	cerr << _("Command 'create-config' needs one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    string subvolume = realpath(getopts.popArg());
    if (subvolume.empty())
    {
	cerr << _("Invalid subvolume.") << endl;
	exit(EXIT_FAILURE);
    }

    string fstype = "";
    string template_name = "default";
    bool add_fstab = false;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("fstype")) != opts.end())
	fstype = opt->second;

    if ((opt = opts.find("template")) != opts.end())
	template_name = opt->second;

    if ((opt = opts.find("add-fstab")) != opts.end())
	add_fstab = true;

    if (fstype.empty() && !Snapper::detectFstype(subvolume, fstype))
    {
	cerr << _("Detecting filesystem type failed.") << endl;
	exit(EXIT_FAILURE);
    }

    if (no_dbus)
    {
	Snapper::createConfig(config_name, subvolume, fstype, template_name, add_fstab);
    }
    else
    {
	command_create_xconfig(*conn, config_name, subvolume, fstype, template_name);
    }
}


void
help_delete_config()
{
    cout << _("  Delete config:") << endl
	 << _("\tsnapper delete-config") << endl
	 << endl;
}


void
command_delete_config(DBus::Connection* conn, Snapper* snapper)
{
    getopts.parse("delete-config", GetOpts::no_options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'delete-config' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    if (no_dbus)
    {
	Snapper::deleteConfig(config_name);
    }
    else
    {
	command_delete_xconfig(*conn, config_name);
    }
}


void
help_get_config()
{
    cout << _("  Get config:") << endl
	 << _("\tsnapper get-config") << endl
	 << endl;
}


void
command_get_config(DBus::Connection* conn, Snapper* snapper)
{
    getopts.parse("get-config", GetOpts::no_options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'get-config' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    Table table;

    TableHeader header;
    header.add(_("Key"));
    header.add(_("Value"));
    table.setHeader(header);

    if (no_dbus)
    {
	ConfigInfo config_info = Snapper::getConfig(config_name);
	map<string, string> raw = config_info.getAllValues();

	for (map<string, string>::const_iterator it = raw.begin(); it != raw.end(); ++it)
	{
	    TableRow row;
	    row.add(it->first);
	    row.add(it->second);
	    table.add(row);
	}
    }
    else
    {
	XConfigInfo ci = command_get_xconfig(*conn, config_name);

	for (map<string, string>::const_iterator it = ci.raw.begin(); it != ci.raw.end(); ++it)
	{
	    TableRow row;
	    row.add(it->first);
	    row.add(it->second);
	    table.add(row);
	}
    }

    cout << table;
}


void
help_set_config()
{
    cout << _("  Set config:") << endl
	 << _("\tsnapper set-config <configdata>") << endl
	 << endl;
}


void
command_set_config(DBus::Connection* conn, Snapper* snapper)
{
    getopts.parse("set-config", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'set-config' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    map<string, string> raw = read_configdata(getopts.getArgs());

    if (no_dbus)
    {
	snapper->setConfigInfo(raw);
    }
    else
    {
	command_set_xconfig(*conn, config_name, raw);
    }
}


void
help_list()
{
    cout << _("  List snapshots:") << endl
	 << _("\tsnapper list") << endl
	 << endl
	 << _("    Options for 'list' command:") << endl
	 << _("\t--type, -t <type>\t\tType of snapshots to list.") << endl
	 << _("\t--all-configs, -a\t\tList snapshots from all accessible configs.") << endl
	 << endl;
}

enum ListMode { LM_ALL, LM_SINGLE, LM_PRE_POST };

void list_from_one_config(DBus::Connection* conn, Snapper* snapper, string config_name, ListMode list_mode);

void
command_list(DBus::Connection* conn, Snapper* snapper)
{
    const struct option options[] = {
	{ "type",		required_argument,	0,	't' },
	{ "all-configs",	no_argument,		0,	'a' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("list", options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'list' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    ListMode list_mode = LM_ALL;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("type")) != opts.end())
    {
	if (opt->second == "all")
	    list_mode = LM_ALL;
	else if (opt->second == "single")
	    list_mode = LM_SINGLE;
	else if (opt->second == "pre-post")
	    list_mode = LM_PRE_POST;
	else
	{
	    cerr << _("Unknown type of snapshots.") << endl;
	    exit(EXIT_FAILURE);
	}
    }

    list<pair<string, string> > configs;
    if ((opt = opts.find("all-configs")) != opts.end())
    {
	configs = enum_configs(conn);
    }
    else
    {
	configs.push_back(make_pair(config_name, ""));
    }

    for (list<pair<string,string> >::iterator it = configs.begin(); it != configs.end(); ++it)
    {
	if (it != configs.begin())
	    cout << endl;

	if (configs.size() > 1)
	    cout << "Config: " << it->first << ", subvolume: " << it->second << endl;
	list_from_one_config(conn, snapper, it->first, list_mode);
    }
}

void list_from_one_config(DBus::Connection* conn, Snapper* snapper, string config_name, ListMode list_mode)
{
    Table table;

    switch (list_mode)
    {
	case LM_ALL:
	{
	    TableHeader header;
	    header.add(_("Type"));
	    header.add(_("#"));
	    header.add(_("Pre #"));
	    header.add(_("Date"));
	    header.add(_("User"));
	    header.add(_("Cleanup"));
	    header.add(_("Description"));
	    header.add(_("Userdata"));
	    table.setHeader(header);

	    if (no_dbus)
	    {
		const Snapshots& snapshots = snapper->getSnapshots();
		for (Snapshots::const_iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
		{
		    TableRow row;
		    row.add(toString(it1->getType()));
		    row.add(decString(it1->getNum()));
		    row.add(it1->getType() == POST ? decString(it1->getPreNum()) : "");
		    row.add(it1->isCurrent() ? "" : datetime(it1->getDate(), utc, iso));
		    row.add(username(it1->getUid()));
		    row.add(it1->getCleanup());
		    row.add(it1->getDescription());
		    row.add(show_userdata(it1->getUserdata()));
		    table.add(row);
		}
	    }
	    else
	    {
		XSnapshots snapshots = command_list_xsnapshots(*conn, config_name);
		for (XSnapshots::const_iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
		{
		    TableRow row;
		    row.add(toString(it1->getType()));
		    row.add(decString(it1->getNum()));
		    row.add(it1->getType() == POST ? decString(it1->getPreNum()) : "");
		    row.add(it1->isCurrent() ? "" : datetime(it1->getDate(), utc, iso));
		    row.add(username(it1->getUid()));
		    row.add(it1->getCleanup());
		    row.add(it1->getDescription());
		    row.add(show_userdata(it1->getUserdata()));
		    table.add(row);
		}
	    }
	}
	break;

	case LM_SINGLE:
	{
	    TableHeader header;
	    header.add(_("#"));
	    header.add(_("Date"));
	    header.add(_("User"));
	    header.add(_("Description"));
	    header.add(_("Userdata"));
	    table.setHeader(header);

	    if (no_dbus)
	    {
		const Snapshots& snapshots = snapper->getSnapshots();
		for (Snapshots::const_iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
		{
		    if (it1->getType() != SINGLE)
			continue;

		    TableRow row;
		    row.add(decString(it1->getNum()));
		    row.add(it1->isCurrent() ? "" : datetime(it1->getDate(), utc, iso));
		    row.add(username(it1->getUid()));
		    row.add(it1->getDescription());
		    row.add(show_userdata(it1->getUserdata()));
		    table.add(row);
		}
	    }
	    else
	    {
		XSnapshots snapshots = command_list_xsnapshots(*conn, config_name);
		for (XSnapshots::const_iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
		{
		    if (it1->getType() != SINGLE)
			continue;

		    TableRow row;
		    row.add(decString(it1->getNum()));
		    row.add(it1->isCurrent() ? "" : datetime(it1->getDate(), utc, iso));
		    row.add(username(it1->getUid()));
		    row.add(it1->getDescription());
		    row.add(show_userdata(it1->getUserdata()));
		    table.add(row);
		}
	    }
	}
	break;

	case LM_PRE_POST:
	{
	    TableHeader header;
	    header.add(_("Pre #"));
	    header.add(_("Post #"));
	    header.add(_("Pre Date"));
	    header.add(_("Post Date"));
	    header.add(_("Description"));
	    header.add(_("Userdata"));
	    table.setHeader(header);

	    if (no_dbus)
	    {
		const Snapshots& snapshots = snapper->getSnapshots();
		for (Snapshots::const_iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
		{
		    if (it1->getType() != PRE)
			continue;

		    Snapshots::const_iterator it2 = snapshots.findPost(it1);
		    if (it2 == snapshots.end())
			continue;

		    TableRow row;
		    row.add(decString(it1->getNum()));
		    row.add(decString(it2->getNum()));
		    row.add(datetime(it1->getDate(), utc, iso));
		    row.add(datetime(it2->getDate(), utc, iso));
		    row.add(it1->getDescription());
		    row.add(show_userdata(it1->getUserdata()));
		    table.add(row);
		}
	    }
	    else
	    {
		XSnapshots snapshots = command_list_xsnapshots(*conn, config_name);
		for (XSnapshots::const_iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
		{
		    if (it1->getType() != PRE)
			continue;

		    XSnapshots::const_iterator it2 = snapshots.findPost(it1);
		    if (it2 == snapshots.end())
			continue;

		    TableRow row;
		    row.add(decString(it1->getNum()));
		    row.add(decString(it2->getNum()));
		    row.add(datetime(it1->getDate(), utc, iso));
		    row.add(datetime(it2->getDate(), utc, iso));
		    row.add(it1->getDescription());
		    row.add(show_userdata(it1->getUserdata()));
		    table.add(row);
		}
	    }
	}
	break;
    }

    cout << table;
}


void
help_create()
{
    cout << _("  Create snapshot:") << endl
	 << _("\tsnapper create") << endl
	 << endl
	 << _("    Options for 'create' command:") << endl
	 << _("\t--type, -t <type>\t\tType for snapshot.") << endl
	 << _("\t--pre-number <number>\t\tNumber of corresponding pre snapshot.") << endl
	 << _("\t--print-number, -p\t\tPrint number of created snapshot.") << endl
	 << _("\t--description, -d <description>\tDescription for snapshot.") << endl
	 << _("\t--cleanup-algorithm, -c <algo>\tCleanup algorithm for snapshot.") << endl
	 << _("\t--userdata, -u <userdata>\tUserdata for snapshot.") << endl
	 << _("\t--command <command>\tRun command and create pre and post snapshots.") << endl
	 << endl;
}


void
command_create(DBus::Connection* conn, Snapper* snapper)
{
    const struct option options[] = {
	{ "type",		required_argument,	0,	't' },
	{ "pre-number",		required_argument,	0,	0 },
	{ "print-number",	no_argument,		0,	'p' },
	{ "description",	required_argument,	0,	'd' },
	{ "cleanup-algorithm",	required_argument,	0,	'c' },
	{ "userdata",		required_argument,	0,	'u' },
	{ "command",		required_argument,	0,	0 },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("create", options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'create' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    enum CreateType { CT_SINGLE, CT_PRE, CT_POST, CT_PRE_POST };

    CreateType type = CT_SINGLE;
    unsigned int num1 = 0;
    bool print_number = false;
    string description;
    string cleanup;
    map<string, string> userdata;
    string command;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("type")) != opts.end())
    {
	if (opt->second == "single")
	    type = CT_SINGLE;
	else if (opt->second == "pre")
	    type = CT_PRE;
	else if (opt->second == "post")
	    type = CT_POST;
	else if (opt->second == "pre-post")
	    type = CT_PRE_POST;
	else
	{
	    cerr << _("Unknown type of snapshot.") << endl;
	    exit(EXIT_FAILURE);
	}
    }

    if ((opt = opts.find("pre-number")) != opts.end())
	num1 = read_num(opt->second);

    if ((opt = opts.find("print-number")) != opts.end())
	print_number = true;

    if ((opt = opts.find("description")) != opts.end())
	description = opt->second;

    if ((opt = opts.find("cleanup-algorithm")) != opts.end())
	cleanup = opt->second;

    if ((opt = opts.find("userdata")) != opts.end())
	userdata = read_userdata(opt->second);

    if ((opt = opts.find("command")) != opts.end())
    {
	command = opt->second;
	type = CT_PRE_POST;
    }

    if (type == CT_POST && (num1 == 0))
    {
	cerr << _("Missing or invalid pre-number.") << endl;
	exit(EXIT_FAILURE);
    }

    if (type == CT_PRE_POST && command.empty())
    {
	cerr << _("Missing command argument.") << endl;
	exit(EXIT_FAILURE);
    }

    switch (type)
    {
	case CT_SINGLE: {
	    unsigned int num1 = command_create_single_xsnapshot(*conn, config_name, description,
								cleanup, userdata);
	    if (print_number)
		cout << num1 << endl;
	} break;

	case CT_PRE: {
	    unsigned int num1 = command_create_pre_xsnapshot(*conn, config_name, description,
							     cleanup, userdata);
	    if (print_number)
		cout << num1 << endl;
	} break;

	case CT_POST: {
	    unsigned int num2 = command_create_post_xsnapshot(*conn, config_name, num1, description,
							      cleanup, userdata);
	    if (print_number)
		cout << num2 << endl;
	} break;

	case CT_PRE_POST: {
	    unsigned int num1 = command_create_pre_xsnapshot(*conn, config_name, description,
							     cleanup, userdata);
	    system(command.c_str());
	    unsigned int num2 = command_create_post_xsnapshot(*conn, config_name, num1, "",
							      cleanup, userdata);
	    if (print_number)
		cout << num1 << ".." << num2 << endl;
	} break;
    }
}


void
help_modify()
{
    cout << _("  Modify snapshot:") << endl
	 << _("\tsnapper modify <number>") << endl
	 << endl
	 << _("    Options for 'modify' command:") << endl
	 << _("\t--description, -d <description>\tDescription for snapshot.") << endl
	 << _("\t--cleanup-algorithm, -c <algo>\tCleanup algorithm for snapshot.") << endl
	 << _("\t--userdata, -u <userdata>\tUserdata for snapshot.") << endl
	 << endl;
}


void
command_modify(DBus::Connection* conn, Snapper* snapper)
{
    const struct option options[] = {
	{ "description",	required_argument,	0,	'd' },
	{ "cleanup-algorithm",	required_argument,	0,	'c' },
	{ "userdata",		required_argument,	0,	'u' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("modify", options);

    if (!getopts.hasArgs())
    {
	cerr << _("Command 'modify' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    while (getopts.hasArgs())
    {
	unsigned int num = read_num(getopts.popArg());

	XSnapshot data = command_get_xsnapshot(*conn, config_name, num);

	GetOpts::parsed_opts::const_iterator opt;

	if ((opt = opts.find("description")) != opts.end())
	    data.description = opt->second;

	if ((opt = opts.find("cleanup-algorithm")) != opts.end())
	    data.cleanup = opt->second;

	if ((opt = opts.find("userdata")) != opts.end())
	    data.userdata = read_userdata(opt->second, data.userdata);

	command_set_xsnapshot(*conn, config_name, num, data);
    }
}


void
help_delete()
{
    cout << _("  Delete snapshot:") << endl
	 << _("\tsnapper delete <number>") << endl
	 << endl;
}


void
command_delete(DBus::Connection* conn, Snapper* snapper)
{
    getopts.parse("delete", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'delete' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    XSnapshots snapshots = command_list_xsnapshots(*conn, config_name);

    list<unsigned int> nums;

    while (getopts.hasArgs())
    {
	string arg = getopts.popArg();

	if (arg.find_first_of("-") == string::npos)
	{
	    unsigned int i = read_num(arg);
	    nums.push_back(i);
	}
	else
	{
	    pair<unsigned int, unsigned int> r(read_nums(arg, "-"));

	    if (r.first > r.second)
		swap(r.first, r.second);

	    for (unsigned int i = r.first; i <= r.second; ++i)
	    {
		if (snapshots.find(i) != snapshots.end() &&
		    find(nums.begin(), nums.end(), i) == nums.end())
		{
		    nums.push_back(i);
		}
	    }
	}
    }

    command_delete_xsnapshots(*conn, config_name, nums);
}


void
help_mount()
{
    cout << _("  Mount snapshot:") << endl
	 << _("\tsnapper mount <number>") << endl
	 << endl;
}


void
command_mount(DBus::Connection* conn, Snapper* snapper)
{
    getopts.parse("mount", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'mount' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    while (getopts.hasArgs())
    {
	if (no_dbus)
	{
	    Snapshots::iterator snapshot = read_num(snapper, getopts.popArg());

            snapshot->mountFilesystemSnapshot(true);
	}
	else
	{
	    unsigned int num = read_num(getopts.popArg());

	    command_mount_xsnapshots(*conn, config_name, num, true);
	}
    }
}


void
help_umount()
{
    cout << _("  Umount snapshot:") << endl
	 << _("\tsnapper umount <number>") << endl
	 << endl;
}


void
command_umount(DBus::Connection* conn, Snapper* snapper)
{
    getopts.parse("mount", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'mount' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    while (getopts.hasArgs())
    {
	if (no_dbus)
	{
	    Snapshots::iterator snapshot = read_num(snapper, getopts.popArg());

            snapshot->umountFilesystemSnapshot(true);
	}
	else
	{
	    unsigned int num = read_num(getopts.popArg());

	    command_umount_xsnapshots(*conn, config_name, num, true);
	}
    }
}


void
help_status()
{
    cout << _("  Comparing snapshots:") << endl
	 << _("\tsnapper status <number1>..<number2>") << endl
	 << endl
	 << _("    Options for 'status' command:") << endl
	 << _("\t--output, -o <file>\t\tSave status to file.") << endl
	 << endl;
}


void
command_status(DBus::Connection* conn, Snapper* snapper)
{
    const struct option options[] = {
	{ "output",		required_argument,	0,	'o' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("status", options);
    if (getopts.numArgs() != 1)
    {
	cerr << _("Command 'status' needs one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    GetOpts::parsed_opts::const_iterator opt;

    pair<unsigned int, unsigned int> nums(read_nums(getopts.popArg()));

    MyComparison comparison(*conn, nums, false);
    MyFiles& files = comparison.files;

    FILE* file = stdout;

    if ((opt = opts.find("output")) != opts.end())
    {
	file = fopen(opt->second.c_str(), "w");
	if (!file)
	{
	    cerr << sformat(_("Opening file '%s' failed."), opt->second.c_str()) << endl;
	    exit(EXIT_FAILURE);
	}
    }

    for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	fprintf(file, "%s %s\n", statusToString(it->getPreToPostStatus()).c_str(),
		it->getAbsolutePath(LOC_SYSTEM).c_str());

    if (file != stdout)
	fclose(file);
}


void
help_diff()
{
    cout << _("  Comparing snapshots:") << endl
	 << _("\tsnapper diff <number1>..<number2> [files]") << endl
	 << endl
	 << _("    Options for 'diff' command:") << endl
	 << _("\t--diff-cmd <command>\t\tCommand used for comparing files.") << endl
	 << _("\t--extensions, -x <options>\tExtra options passed to the diff command.") << endl
	 << endl;
}


void
command_diff(DBus::Connection* conn, Snapper* snapper)
{
    const struct option options[] = {
	{ "diff-cmd",		required_argument,	0,	0 },
	{ "extensions",		required_argument,	0,	'x' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("diff", options);

    if (getopts.numArgs() < 1)
    {
	cerr << _("Command 'diff' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    Differ differ;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("diff-cmd")) != opts.end())
	differ.command = opt->second;

    if ((opt = opts.find("extensions")) != opts.end())
	differ.extensions = opt->second;

    pair<unsigned int, unsigned int> nums(read_nums(getopts.popArg()));

    MyComparison comparison(*conn, nums, true);
    MyFiles& files = comparison.files;

    if (getopts.numArgs() == 0)
    {
	for (Files::const_iterator it1 = files.begin(); it1 != files.end(); ++it1)
	    differ.run(it1->getAbsolutePath(LOC_PRE), it1->getAbsolutePath(LOC_POST));
    }
    else
    {
	while (getopts.numArgs() > 0)
	{
	    string name = getopts.popArg();

	    Files::const_iterator it1 = files.findAbsolutePath(name);
	    if (it1 != files.end())
		differ.run(it1->getAbsolutePath(LOC_PRE), it1->getAbsolutePath(LOC_POST));
	}
    }
}


void
help_undo()
{
    cout << _("  Undo changes:") << endl
	 << _("\tsnapper undochange <number1>..<number2> [files]") << endl
	 << endl
	 << _("    Options for 'undochange' command:") << endl
	 << _("\t--input, -i <file>\t\tRead files for which to undo changes from file.") << endl
	 << endl;
}


void
command_undo(DBus::Connection* conn, Snapper* snapper)
{
    const struct option options[] = {
	{ "input",		required_argument,	0,	'i' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("undochange", options);
    if (getopts.numArgs() < 1)
    {
	cerr << _("Command 'undochange' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    pair<unsigned int, unsigned int> nums(read_nums(getopts.popArg()));

    FILE* file = NULL;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("input")) != opts.end())
    {
	file = fopen(opt->second.c_str(), "r");
	if (!file)
	{
	    cerr << sformat(_("Opening file '%s' failed."), opt->second.c_str()) << endl;
	    exit(EXIT_FAILURE);
	}
    }

    if (nums.first == 0)
    {
	cerr << _("Invalid snapshots.") << endl;
	exit(EXIT_FAILURE);
    }

    MyComparison comparison(*conn, nums, true);
    MyFiles& files = comparison.files;

    if (file)
    {
	AsciiFileReader asciifile(file);

	string line;
	while (asciifile.getline(line))
	{
	    if (line.empty())
		continue;

	    string name = line;

	    // strip optional status
	    if (name[0] != '/')
	    {
		string::size_type pos = name.find(" ");
		if (pos == string::npos)
		    continue;

		name.erase(0, pos + 1);
	    }

	    Files::iterator it = files.findAbsolutePath(name);
	    if (it == files.end())
            {
                cerr << sformat(_("File '%s' not found."), name.c_str()) << endl;
                exit(EXIT_FAILURE);
            }

	    it->setUndo(true);
	}
    }
    else
    {
	if (getopts.numArgs() == 0)
	{
	    for (Files::iterator it = files.begin(); it != files.end(); ++it)
		it->setUndo(true);
	}
	else
	{
	    while (getopts.numArgs() > 0)
	    {
		Files::iterator it = files.findAbsolutePath(getopts.popArg());
		if (it == files.end())
                    continue;

		it->setUndo(true);
	    }
	}
    }

    UndoStatistic undo_statistic = files.getUndoStatistic();

    if (undo_statistic.empty())
    {
	cout << _("nothing to do") << endl;
	return;
    }

    cout << sformat(_("create:%d modify:%d delete:%d"), undo_statistic.numCreate,
		    undo_statistic.numModify, undo_statistic.numDelete) << endl;

    vector<UndoStep> undo_steps = files.getUndoSteps();

    for (vector<UndoStep>::const_iterator it1 = undo_steps.begin(); it1 != undo_steps.end(); ++it1)
    {
	vector<File>::iterator it2 = files.find(it1->name);
	if (it2 == files.end())
	{
	    cerr << "internal error" << endl;
	    exit(EXIT_FAILURE);
	}

	if (it1->action != it2->getAction())
	{
	    cerr << "internal error" << endl;
	    exit(EXIT_FAILURE);
	}

	if (verbose)
	{
	    switch (it1->action)
	    {
		case CREATE:
		    cout << sformat(_("creating %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
		    break;
		case MODIFY:
		    cout << sformat(_("modifying %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
		    break;
		case DELETE:
		    cout << sformat(_("deleting %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
		    break;
	    }
	}

	if (!it2->doUndo())
	{
	    switch (it1->action)
	    {
		case CREATE:
		    cerr << sformat(_("failed to create %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
		    break;
		case MODIFY:
		    cerr << sformat(_("failed to modify %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
		    break;
		case DELETE:
		    cerr << sformat(_("failed to delete %s"), it2->getAbsolutePath(LOC_SYSTEM).c_str()) << endl;
		    break;
	    }
	}
    }
}


#ifdef ENABLE_ROLLBACK

const Filesystem*
getFilesystem(DBus::Connection* conn, Snapper* snapper)
{
    XConfigInfo ci = command_get_xconfig(*conn, config_name);

    map<string, string>::const_iterator it = ci.raw.find(KEY_FSTYPE);
    if (it == ci.raw.end())
    {
	cerr << _("Failed to initialize filesystem handler.") << endl;
	exit(EXIT_FAILURE);
    }

    try
    {
	return Filesystem::create(it->second, ci.subvolume);
    }
    catch (const InvalidConfigException& e)
    {
	cerr << _("Failed to initialize filesystem handler.") << endl;
	exit(EXIT_FAILURE);
    }
}


void
help_rollback()
{
    cout << _("  Rollback:") << endl
	 << _("\tsnapper rollback [number]") << endl
	 << endl
	 << _("    Options for 'rollback' command:") << endl
	 << _("\t--print-number, -p\t\tPrint number of second created snapshot.") << endl
	 << _("\t--description, -d <description>\tDescription for snapshots.") << endl
	 << _("\t--cleanup-algorithm, -c <algo>\tCleanup algorithm for snapshots.") << endl
	 << _("\t--userdata, -u <userdata>\tUserdata for snapshots.") << endl
	 << endl;
}


void
command_rollback(DBus::Connection* conn, Snapper* snapper)
{
    const struct option options[] = {
	{ "print-number",       no_argument,            0,      'p' },
	{ "description",        required_argument,      0,      'd' },
	{ "cleanup-algorithm",  required_argument,      0,      'c' },
	{ "userdata",           required_argument,      0,      'u' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("rollback", options);
    if (getopts.hasArgs() && getopts.numArgs() != 1)
    {
	cerr << _("Command 'rollback' takes either one or no argument.") << endl;
	exit(EXIT_FAILURE);
    }

    bool print_number = false;
    string description;
    string cleanup;
    map<string, string> userdata;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("print-number")) != opts.end())
	print_number = true;

    if ((opt = opts.find("description")) != opts.end())
	description = opt->second;

    if ((opt = opts.find("cleanup-algorithm")) != opts.end())
	cleanup = opt->second;

    if ((opt = opts.find("userdata")) != opts.end())
	userdata = read_userdata(opt->second);

    const Filesystem* filesystem = getFilesystem(conn, snapper);
    if (filesystem->fstype() != "btrfs")
    {
	cerr << _("Command 'rollback' only available for btrfs.") << endl;
	exit(EXIT_FAILURE);
    }

    unsigned int num2;

    if (getopts.numArgs() == 0)
    {
	if (!quiet)
	    cout << _("Creating read-only snapshot of default subvolume.") << flush;
	unsigned int num1 = command_create_single_xsnapshot_of_default(*conn, config_name, true,
								       description, cleanup,
								       userdata);
	if (!quiet)
	    cout << " " << sformat(_("(Snapshot %d.)"), num1) << endl;

	if (!quiet)
	    cout << _("Creating read-write snapshot of current subvolume.") <<flush;
	num2 = command_create_single_xsnapshot_v2(*conn, config_name, 0, false, description,
						  cleanup, userdata);
	if (!quiet)
	    cout << " " << sformat(_("(Snapshot %d.)"), num2) << endl;
    }
    else
    {
	unsigned int tmp = read_num(getopts.popArg());

	if (!quiet)
	    cout << _("Creating read-only snapshot of current system.") << flush;
	unsigned int num1 = command_create_single_xsnapshot(*conn, config_name, description,
							    cleanup, userdata);
	if (!quiet)
	    cout << " " << sformat(_("(Snapshot %d.)"), num1) << endl;

	if (!quiet)
	    cout << sformat(_("Creating read-write snapshot of snapshot %d."), tmp) << flush;
	num2 = command_create_single_xsnapshot_v2(*conn, config_name, tmp, false,
						  description, cleanup, userdata);
	if (!quiet)
	    cout << " " << sformat(_("(Snapshot %d.)"), num2) << endl;
    }

    if (!quiet)
	cout << sformat(_("Setting default subvolume to snapshot %d."), num2) << endl;
    filesystem->setDefault(num2);

    if (print_number)
	cout << num2 << endl;
}

#endif


void
help_cleanup()
{
    cout << _("  Cleanup snapshots:") << endl
	 << _("\tsnapper cleanup <cleanup-algorithm>") << endl
	 << endl;
}


void
command_cleanup(DBus::Connection* conn, Snapper* snapper)
{
    GetOpts::parsed_opts opts = getopts.parse("cleanup", GetOpts::no_options);
    if (getopts.numArgs() != 1)
    {
	cerr << _("Command 'cleanup' needs one arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    string cleanup = getopts.popArg();

    if (cleanup == "number")
    {
	do_cleanup_number(*conn, config_name);
    }
    else if (cleanup == "timeline")
    {
	do_cleanup_timeline(*conn, config_name);
    }
    else if (cleanup == "empty-pre-post")
    {
	do_cleanup_empty_pre_post(*conn, config_name);
    }
    else
    {
	cerr << sformat(_("Unknown cleanup algorithm '%s'."), cleanup.c_str()) << endl;
	exit(EXIT_FAILURE);
    }
}


void
help_debug()
{
}


void
command_debug(DBus::Connection* conn, Snapper* snapper)
{
    getopts.parse("debug", GetOpts::no_options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'debug' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    vector<string> lines = command_xdebug(*conn);
    for (vector<string>::const_iterator it = lines.begin(); it != lines.end(); ++it)
	cout << *it << endl;
}


#ifdef ENABLE_XATTRS

void
help_xa_diff()
{
    cout << _("  Comparing snapshots extended attributes:") << endl
         << _("\tsnapper xadiff <number1>..<number2> [files]") << endl
         << endl;
}

void
print_xa_diff(const string loc_pre, const string loc_post)
{
    try {
        XAModification xa_mod = XAModification(XAttributes(loc_pre), XAttributes(loc_post));

        if (!xa_mod.empty())
	{
	    cout << "--- " << loc_pre << endl << "+++ " << loc_post << endl;
	    xa_mod.dumpDiffReport(cout);
	}
    }
    catch (const XAttributesException& e) {}
}

void
command_xa_diff(DBus::Connection* conn, Snapper* snapper)
{
    GetOpts::parsed_opts opts = getopts.parse("xadiff", GetOpts::no_options);

    if (getopts.numArgs() < 1)
    {
        cerr << _("Command 'xadiff' needs at least one argument.") << endl;
        exit(EXIT_FAILURE);
    }

    pair<unsigned int, unsigned int> nums(read_nums(getopts.popArg()));

    MyComparison comparison(*conn, nums, true);
    MyFiles& files = comparison.files;

    if (getopts.numArgs() == 0)
    {
	for (Files::const_iterator it1 = files.begin(); it1 != files.end(); ++it1)
            if (it1->getPreToPostStatus() & XATTRS)
                print_xa_diff(it1->getAbsolutePath(LOC_PRE), it1->getAbsolutePath(LOC_POST));
    }
    else
    {
        while (getopts.numArgs() > 0)
        {
            string name = getopts.popArg();

            Files::const_iterator it1 = files.findAbsolutePath(name);
            if (it1 == files.end())
                continue;

            if (it1->getPreToPostStatus() & XATTRS)
                print_xa_diff(it1->getAbsolutePath(LOC_PRE), it1->getAbsolutePath(LOC_POST));
        }
    }
}

#endif


void
log_do(LogLevel level, const string& component, const char* file, const int line, const char* func,
       const string& text)
{
    cerr << text << endl;
}


bool
log_query(LogLevel level, const string& component)
{
    return level == ERROR;
}


void usage() __attribute__ ((__noreturn__));

void
usage()
{
    cerr << "Try 'snapper --help' for more information." << endl;
    exit(EXIT_FAILURE);
}


void help() __attribute__ ((__noreturn__));

void
help(const list<Cmd>& cmds)
{
    getopts.parse("help", GetOpts::no_options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'help' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    cout << _("usage: snapper [--global-options] <command> [--command-options] [command-arguments]") << endl
	 << endl;

    cout << _("    Global options:") << endl
	 << _("\t--quiet, -q\t\t\tSuppress normal output.") << endl
	 << _("\t--verbose, -v\t\t\tIncrease verbosity.") << endl
	 << _("\t--utc\t\t\t\tDisplay dates and times in UTC.") << endl
	 << _("\t--iso\t\t\t\tDisplay dates and times in ISO format.") << endl
	 << _("\t--table-style, -t <style>\tTable style (integer).") << endl
	 << _("\t--config, -c <name>\t\tSet name of config to use.") << endl
	 << _("\t--no-dbus\t\t\tOperate without DBus.") << endl
	 << _("\t--version\t\t\tPrint version and exit.") << endl
	 << endl;

    for (list<Cmd>::const_iterator cmd = cmds.begin(); cmd != cmds.end(); ++cmd)
	(*cmd->help_func)();

    exit (EXIT_SUCCESS);
}


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    setLogDo(&log_do);
    setLogQuery(&log_query);

    const list<Cmd> cmds = {
	Cmd("list-configs", command_list_configs, help_list_configs, true, false),
	Cmd("create-config", command_create_config, help_create_config, true, false),
	Cmd("delete-config", command_delete_config, help_delete_config, true, false),
	Cmd("get-config", command_get_config, help_get_config, true, false),
	Cmd("set-config", command_set_config, help_set_config, true, true),
	Cmd("list", { "ls" }, command_list, help_list, true, true),
	Cmd("create", command_create, help_create, false, true),
	Cmd("modify", command_modify, help_modify, false, true),
	Cmd("delete", { "remove", "rm" }, command_delete, help_delete, false, true),
	Cmd("mount", command_mount, help_mount, true, true),
	Cmd("umount", command_umount, help_umount, true, true),
	Cmd("status", command_status, help_status, false, true),
	Cmd("diff", command_diff, help_diff, false, true),
#ifdef ENABLE_XATTRS
	Cmd("xadiff", command_xa_diff, help_xa_diff, false, true),
#endif
	Cmd("undochange", command_undo, help_undo, false, true),
#ifdef ENABLE_ROLLBACK
	Cmd("rollback", command_rollback, help_rollback, false, true),
#endif
	Cmd("cleanup", command_cleanup, help_cleanup, false, true),
	Cmd("debug", command_debug, help_debug, false, false)
    };

    const struct option options[] = {
	{ "quiet",		no_argument,		0,	'q' },
	{ "verbose",		no_argument,		0,	'v' },
	{ "utc",		no_argument,		0,	0 },
	{ "iso",		no_argument,		0,	0 },
	{ "table-style",	required_argument,	0,	't' },
	{ "config",		required_argument,	0,	'c' },
	{ "no-dbus",		no_argument,		0,	0 },
	{ "version",		no_argument,		0,	0 },
	{ "help",		no_argument,		0,	0 },
	{ 0, 0, 0, 0 }
    };

    getopts.init(argc, argv);

    GetOpts::parsed_opts opts = getopts.parse(options);

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("quiet")) != opts.end())
	quiet = true;

    if ((opt = opts.find("verbose")) != opts.end())
	verbose = true;

    if ((opt = opts.find("utc")) != opts.end())
	utc = true;

    if ((opt = opts.find("iso")) != opts.end())
	iso = true;

    if ((opt = opts.find("table-style")) != opts.end())
    {
	unsigned int s;
	opt->second >> s;
	if (s >= Table::numStyles)
	{
	    cerr << sformat(_("Invalid table style %d."), s) << " "
		 << sformat(_("Use an integer number from %d to %d."), 0, Table::numStyles - 1) << endl;
	    exit(EXIT_FAILURE);
	}
	Table::defaultStyle = (TableLineStyle) s;
    }

    if ((opt = opts.find("config")) != opts.end())
	config_name = opt->second;

    if ((opt = opts.find("no-dbus")) != opts.end())
	no_dbus = true;

    if ((opt = opts.find("version")) != opts.end())
    {
	cout << "snapper " << Snapper::compileVersion() << endl;
	cout << "flags " << Snapper::compileFlags() << endl;
	exit(EXIT_SUCCESS);
    }

    if ((opt = opts.find("help")) != opts.end())
    {
	help(cmds);
    }

    if (!getopts.hasArgs())
    {
	cerr << _("No command provided.") << endl
	     << _("Try 'snapper --help' for more information.") << endl;
	exit(EXIT_FAILURE);
    }

    const char* command = getopts.popArg();

    list<Cmd>::const_iterator cmd = cmds.begin();
    while (cmd != cmds.end() && (cmd->name != command && !contains(cmd->aliases, command)))
	++cmd;

    if (cmd == cmds.end())
    {
	cerr << sformat(_("Unknown command '%s'."), command) << endl
	     << _("Try 'snapper --help' for more information.") << endl;
	exit(EXIT_FAILURE);
    }

    if (no_dbus)
    {
	if (!cmd->works_without_dbus)
	{
	    cerr << sformat(_("Command '%s' does not work without DBus."), cmd->name.c_str()) << endl;
	    exit(EXIT_FAILURE);
	}

	try
	{
	    Snapper* snapper = cmd->needs_snapper ? new Snapper(config_name) : NULL;

	    (*cmd->cmd_func)(NULL, snapper);

	    delete snapper;
	}

	catch (const ConfigNotFoundException& e)
        {
            cerr << sformat(_("Config '%s' not found."), config_name.c_str()) << endl;
            exit(EXIT_FAILURE);
        }
        catch (const InvalidConfigException& e)
        {
            cerr << sformat(_("Config '%s' is invalid."), config_name.c_str()) << endl;
            exit(EXIT_FAILURE);
        }
	catch (const ListConfigsFailedException& e)
	{
	    cerr << sformat(_("Listing configs failed (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const CreateConfigFailedException& e)
	{
	    cerr << sformat(_("Creating config failed (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const DeleteConfigFailedException& e)
	{
	    cerr << sformat(_("Deleting config failed (%s)."), e.what()) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const InvalidConfigdataException& e)
	{
	    cerr << _("Invalid configdata.") << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const AclException& e)
	{
	    cerr << _("ACL error.") << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const InvalidUserException& e)
	{
	    cerr << _("Invalid user.") << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const InvalidGroupException& e)
	{
	    cerr << _("Invalid group.") << endl;
	    exit(EXIT_FAILURE);
	}
    }
    else
    {
	try
	{
	    DBus::Connection conn(DBUS_BUS_SYSTEM);

	    (*cmd->cmd_func)(&conn, NULL);
	}
	catch (const DBus::ErrorException& e)
	{
	    cerr << error_description(e) << endl;
	    exit(EXIT_FAILURE);
	}
	catch (const DBus::FatalException& e)
	{
	    cerr << _("Failure") << " (" << e.what() << ")." << endl;
	    exit(EXIT_FAILURE);
	}
    }

    exit(EXIT_SUCCESS);
}
