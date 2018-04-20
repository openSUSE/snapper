/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2017] SUSE LLC
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
#include <snapper/Hooks.h>
#endif

#include "utils/text.h"
#include "utils/Table.h"
#include "utils/GetOpts.h"

#include "cleanup.h"
#include "errors.h"
#include "proxy.h"
#include "misc.h"


using namespace snapper;
using namespace std;


struct Cmd
{
    typedef void (*cmd_func_t)(ProxySnappers* snappers, ProxySnapper* snapper);
    typedef void (*help_func_t)();

    Cmd(const string& name, cmd_func_t cmd_func, help_func_t help_func, bool needs_snapper)
	: name(name), aliases(), cmd_func(cmd_func), help_func(help_func),
	  needs_snapper(needs_snapper)
    {}

    Cmd(const string& name, const vector<string>& aliases, cmd_func_t cmd_func,
	help_func_t help_func, bool needs_snapper)
	: name(name), aliases(aliases), cmd_func(cmd_func), help_func(help_func),
	  needs_snapper(needs_snapper)
    {}

    const string name;
    const vector<string> aliases;
    const cmd_func_t cmd_func;
    const help_func_t help_func;
    const bool needs_snapper;
};


GetOpts getopts;

bool quiet = false;
bool verbose = false;
bool utc = false;
bool iso = false;
string config_name = "root";
bool no_dbus = false;
string target_root = "/";


struct MyFiles : public Files
{

    MyFiles(const Files& files) : Files(files) {}

    void bulk_process(FILE* file, std::function<void(File& file)> callback);

};


void
MyFiles::bulk_process(FILE* file, std::function<void(File& file)> callback)
{
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

	    Files::iterator it = findAbsolutePath(name);
	    if (it == end())
	    {
		cerr << sformat(_("File '%s' not found."), name.c_str()) << endl;
		exit(EXIT_FAILURE);
	    }

	    callback(*it);
	}
    }
    else
    {
	if (getopts.numArgs() == 0)
	{
	    for (Files::iterator it = begin(); it != end(); ++it)
		callback(*it);
	}
	else
	{
	    while (getopts.numArgs() > 0)
	    {
		string name = getopts.popArg();

		Files::iterator it = findAbsolutePath(name);
		if (it == end())
		{
		    cerr << sformat(_("File '%s' not found."), name.c_str()) << endl;
		    exit(EXIT_FAILURE);
		}

		callback(*it);
	    }
	}
    }
}


void
help_list_configs()
{
    cout << _("  List configs:") << endl
	 << _("\tsnapper list-configs") << endl
	 << endl;
}


void
command_list_configs(ProxySnappers* snappers, ProxySnapper*)
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

    map<string, ProxyConfig> configs = snappers->getConfigs();
    for (const map<string, ProxyConfig>::value_type value : configs)
    {
	TableRow row;
	row.add(value.first);
	row.add(value.second.getSubvolume());
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
command_create_config(ProxySnappers* snappers, ProxySnapper*)
{
    const struct option options[] = {
	{ "fstype",		required_argument,	0,	'f' },
	{ "template",		required_argument,	0,	't' },
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

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("fstype")) != opts.end())
	fstype = opt->second;

    if ((opt = opts.find("template")) != opts.end())
	template_name = opt->second;

    if (fstype.empty() && !Snapper::detectFstype(subvolume, fstype))
    {
	cerr << _("Detecting filesystem type failed.") << endl;
	exit(EXIT_FAILURE);
    }

    snappers->createConfig(config_name, subvolume, fstype, template_name);
}


void
help_delete_config()
{
    cout << _("  Delete config:") << endl
	 << _("\tsnapper delete-config") << endl
	 << endl;
}


void
command_delete_config(ProxySnappers* snappers, ProxySnapper*)
{
    getopts.parse("delete-config", GetOpts::no_options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'delete-config' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    snappers->deleteConfig(config_name);
}


void
help_get_config()
{
    cout << _("  Get config:") << endl
	 << _("\tsnapper get-config") << endl
	 << endl;
}


void
command_get_config(ProxySnappers* snappers, ProxySnapper* snapper)
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

    ProxyConfig config = snapper->getConfig();
    for (const map<string, string>::value_type& value : config.getAllValues())
    {
	TableRow row;
	row.add(value.first);
	row.add(value.second);
	table.add(row);
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
command_set_config(ProxySnappers* snappers, ProxySnapper* snapper)
{
    getopts.parse("set-config", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'set-config' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    ProxyConfig config(read_configdata(getopts.getArgs()));

    snapper->setConfig(config);
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


void
list_from_one_config(ProxySnapper* snapper, ListMode list_mode);


void
command_list(ProxySnappers* snappers, ProxySnapper*)
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

    vector<string> tmp;

    if ((opt = opts.find("all-configs")) == opts.end())
    {
        tmp.push_back(config_name);
    }
    else
    {
        map<string, ProxyConfig> configs = snappers->getConfigs();
        for (map<string, ProxyConfig>::value_type it : configs)
            tmp.push_back(it.first);
    }

    for (vector<string>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
	ProxySnapper* snapper = snappers->getSnapper(*it);

        if (it != tmp.begin())
            cout << endl;

        if (tmp.size() > 1)
        {
            cout << "Config: " << snapper->configName() << ", subvolume: "
                 << snapper->getConfig().getSubvolume() << endl;
        }

        list_from_one_config(snapper, list_mode);
    }
}


void
list_from_one_config(ProxySnapper* snapper, ListMode list_mode)
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

	    const ProxySnapshots& snapshots = snapper->getSnapshots();
	    for (const ProxySnapshot& snapshot : snapshots)
	    {
		TableRow row;
		row.add(toString(snapshot.getType()));
		row.add(decString(snapshot.getNum()));
		row.add(snapshot.getType() == POST ? decString(snapshot.getPreNum()) : "");
		row.add(snapshot.isCurrent() ? "" : datetime(snapshot.getDate(), utc, iso));
		row.add(username(snapshot.getUid()));
		row.add(snapshot.getCleanup());
		row.add(snapshot.getDescription());
		row.add(show_userdata(snapshot.getUserdata()));
		table.add(row);
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

	    const ProxySnapshots& snapshots = snapper->getSnapshots();
	    for (const ProxySnapshot& snapshot : snapshots)
	    {
		if (snapshot.getType() != SINGLE)
		    continue;

		TableRow row;
		row.add(decString(snapshot.getNum()));
		row.add(snapshot.isCurrent() ? "" : datetime(snapshot.getDate(), utc, iso));
		row.add(username(snapshot.getUid()));
		row.add(snapshot.getDescription());
		row.add(show_userdata(snapshot.getUserdata()));
		table.add(row);
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

	    const ProxySnapshots& snapshots = snapper->getSnapshots();
	    for (ProxySnapshots::const_iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
	    {
		if (it1->getType() != PRE)
		    continue;

		ProxySnapshots::const_iterator it2 = snapshots.findPost(it1);
		if (it2 == snapshots.end())
		    continue;

		const ProxySnapshot& pre = *it1;
		const ProxySnapshot& post = *it2;

		TableRow row;
		row.add(decString(pre.getNum()));
		row.add(decString(post.getNum()));
		row.add(datetime(pre.getDate(), utc, iso));
		row.add(datetime(post.getDate(), utc, iso));
		row.add(pre.getDescription());
		row.add(show_userdata(pre.getUserdata()));
		table.add(row);
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
command_create(ProxySnappers* snappers, ProxySnapper* snapper)
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

    const ProxySnapshots& snapshots = snapper->getSnapshots();

    CreateType type = CT_SINGLE;
    ProxySnapshots::const_iterator snapshot1 = snapshots.end();
    ProxySnapshots::const_iterator snapshot2 = snapshots.end();
    bool print_number = false;
    SCD scd;
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
	snapshot1 = snapshots.findNum(opt->second);

    if ((opt = opts.find("print-number")) != opts.end())
	print_number = true;

    if ((opt = opts.find("description")) != opts.end())
	scd.description = opt->second;

    if ((opt = opts.find("cleanup-algorithm")) != opts.end())
	scd.cleanup = opt->second;

    if ((opt = opts.find("userdata")) != opts.end())
	scd.userdata = read_userdata(opt->second);

    if ((opt = opts.find("command")) != opts.end())
    {
	command = opt->second;
	type = CT_PRE_POST;
    }

    if (type == CT_POST && snapshot1 == snapshots.end())
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
	    snapshot1 = snapper->createSingleSnapshot(scd);
	    if (print_number)
		cout << snapshot1->getNum() << endl;
	} break;

	case CT_PRE: {
	    snapshot1 = snapper->createPreSnapshot(scd);
	    if (print_number)
		cout << snapshot1->getNum() << endl;
	} break;

	case CT_POST: {
	    snapshot2 = snapper->createPostSnapshot(snapshot1, scd);
	    if (print_number)
		cout << snapshot2->getNum() << endl;
	} break;

	case CT_PRE_POST: {
	    snapshot1 = snapper->createPreSnapshot(scd);
	    system(command.c_str());
	    snapshot2 = snapper->createPostSnapshot(snapshot1, scd);
	    if (print_number)
		cout << snapshot1->getNum() << ".." << snapshot2->getNum() << endl;
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
command_modify(ProxySnappers* snappers, ProxySnapper* snapper)
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

    ProxySnapshots& snapshots = snapper->getSnapshots();

    while (getopts.hasArgs())
    {
	ProxySnapshots::iterator snapshot = snapshots.findNum(getopts.popArg());

	SMD smd = snapshot->getSmd();

	GetOpts::parsed_opts::const_iterator opt;

	if ((opt = opts.find("description")) != opts.end())
	    smd.description = opt->second;

	if ((opt = opts.find("cleanup-algorithm")) != opts.end())
	    smd.cleanup = opt->second;

	if ((opt = opts.find("userdata")) != opts.end())
	    smd.userdata = read_userdata(opt->second, snapshot->getUserdata());

	snapper->modifySnapshot(snapshot, smd);
    }
}


void
help_delete()
{
    cout << _("  Delete snapshot:") << endl
	 << _("\tsnapper delete <number>") << endl
	 << endl
	 << _("    Options for 'delete' command:") << endl
	 << _("\t--sync, -s\t\t\tSync after deletion.") << endl
	 << endl;
}


void
command_delete(ProxySnappers* snappers, ProxySnapper* snapper)
{
    const struct option options[] = {
	{ "sync",		no_argument,		0,	's' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("delete", options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'delete' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    bool sync = false;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("sync")) != opts.end())
	sync = true;

    ProxySnapshots& snapshots = snapper->getSnapshots();

    vector<ProxySnapshots::iterator> nums;

    while (getopts.hasArgs())
    {
	string arg = getopts.popArg();

	if (arg.find_first_of("-") == string::npos)
	{
	    ProxySnapshots::iterator tmp = snapshots.findNum(arg);
	    nums.push_back(tmp);
	}
	else
	{
	    pair<ProxySnapshots::iterator, ProxySnapshots::iterator> range =
		snapshots.findNums(arg, "-");

	    if (range.first->getNum() > range.second->getNum())
		swap(range.first, range.second);

	    for (unsigned int i = range.first->getNum(); i <= range.second->getNum(); ++i)
	    {
		ProxySnapshots::iterator x = snapshots.find(i);
		if (x != snapshots.end())
		{
		    if (find_if(nums.begin(), nums.end(), [i](ProxySnapshots::iterator it)
				{ return it->getNum() == i; }) == nums.end())
			nums.push_back(x);
		}
	    }
	}
    }

    snapper->deleteSnapshots(nums, verbose);

    if (sync)
	snapper->syncFilesystem();
}


void
help_mount()
{
    cout << _("  Mount snapshot:") << endl
	 << _("\tsnapper mount <number>") << endl
	 << endl;
}


void
command_mount(ProxySnappers* snappers, ProxySnapper* snapper)
{
    getopts.parse("mount", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'mount' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    const ProxySnapshots& snapshots = snapper->getSnapshots();

    while (getopts.hasArgs())
    {
	ProxySnapshots::const_iterator snapshot = snapshots.findNum(getopts.popArg());
	snapshot->mountFilesystemSnapshot(true);
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
command_umount(ProxySnappers* snappers, ProxySnapper* snapper)
{
    getopts.parse("umount", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'umount' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    const ProxySnapshots& snapshots = snapper->getSnapshots();

    while (getopts.hasArgs())
    {
	ProxySnapshots::const_iterator snapshot = snapshots.findNum(getopts.popArg());
	snapshot->umountFilesystemSnapshot(true);
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
command_status(ProxySnappers* snappers, ProxySnapper* snapper)
{
    const struct option options[] = {
	{ "output",		required_argument,	0,	'o' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("status", options);
    if (getopts.numArgs() != 1)
    {
	cerr << _("Command 'status' needs one argument.") << endl;

	if (getopts.numArgs() == 2)
	{
	    cerr << _("Maybe you forgot the delimiter '..' between the snapshot numbers.") << endl
		 << _("See 'man snapper' for further instructions.") << endl;
	}

	exit(EXIT_FAILURE);
    }

    GetOpts::parsed_opts::const_iterator opt;

    ProxySnapshots& snapshots = snapper->getSnapshots();

    pair<ProxySnapshots::const_iterator, ProxySnapshots::const_iterator> range =
	snapshots.findNums(getopts.popArg());

    ProxyComparison comparison = snapper->createComparison(*range.first, *range.second, false);

    MyFiles files(comparison.getFiles());

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
	 << _("\t--input, -i <file>\t\tRead files to diff from file.") << endl
	 << _("\t--diff-cmd <command>\t\tCommand used for comparing files.") << endl
	 << _("\t--extensions, -x <options>\tExtra options passed to the diff command.") << endl
	 << endl;
}


void
command_diff(ProxySnappers* snappers, ProxySnapper* snapper)
{
    const struct option options[] = {
	{ "input",		required_argument,	0,	'i' },
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

    FILE* file = NULL;
    Differ differ;

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

    if ((opt = opts.find("diff-cmd")) != opts.end())
	differ.command = opt->second;

    if ((opt = opts.find("extensions")) != opts.end())
	differ.extensions = opt->second;

    ProxySnapshots& snapshots = snapper->getSnapshots();

    pair<ProxySnapshots::const_iterator, ProxySnapshots::const_iterator> range =
	snapshots.findNums(getopts.popArg());

    ProxyComparison comparison = snapper->createComparison(*range.first, *range.second, true);

    MyFiles files(comparison.getFiles());

    files.bulk_process(file, [differ](const File& file) {
	differ.run(file.getAbsolutePath(LOC_PRE), file.getAbsolutePath(LOC_POST));
    });
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
command_undo(ProxySnappers* snappers, ProxySnapper* snapper)
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

    ProxySnapshots& snapshots = snapper->getSnapshots();

    pair<ProxySnapshots::const_iterator, ProxySnapshots::const_iterator> range =
	snapshots.findNums(getopts.popArg());

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

    if (range.first->isCurrent())
    {
	cerr << _("Invalid snapshots.") << endl;
	exit(EXIT_FAILURE);
    }

    ProxyComparison comparison = snapper->createComparison(*range.first, *range.second, true);

    MyFiles files(comparison.getFiles());

    files.bulk_process(file, [](File& file) {
	file.setUndo(true);
    });

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
getFilesystem(const ProxyConfig& config)
{
    const map<string, string>& raw = config.getAllValues();

    map<string, string>::const_iterator pos1 = raw.find(KEY_FSTYPE);
    map<string, string>::const_iterator pos2 = raw.find(KEY_SUBVOLUME);
    if (pos1 == raw.end() || pos2 == raw.end())
    {
	cerr << _("Failed to initialize filesystem handler.") << endl;
	exit(EXIT_FAILURE);
    }

    try
    {
	return Filesystem::create(pos1->second, pos2->second, target_root);
    }
    catch (const InvalidConfigException& e)
    {
	SN_CAUGHT(e);
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
command_rollback(ProxySnappers* snappers, ProxySnapper* snapper)
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

    SCD scd1;
    scd1.description = "rollback backup";
    scd1.cleanup = "number";
    scd1.userdata["important"] = "yes";

    SCD scd2;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("print-number")) != opts.end())
	print_number = true;

    if ((opt = opts.find("description")) != opts.end())
	scd1.description = scd2.description = opt->second;

    if ((opt = opts.find("cleanup-algorithm")) != opts.end())
	scd1.cleanup = scd2.cleanup = opt->second;

    if ((opt = opts.find("userdata")) != opts.end())
    {
	scd1.userdata = read_userdata(opt->second, scd1.userdata);
	scd2.userdata = read_userdata(opt->second);
    }

    ProxyConfig config = snapper->getConfig();

    const Filesystem* filesystem = getFilesystem(config);
    if (filesystem->fstype() != "btrfs")
    {
	cerr << _("Command 'rollback' only available for btrfs.") << endl;
	exit(EXIT_FAILURE);
    }

    const string subvolume = config.getSubvolume();
    if (subvolume != "/")
    {
	cerr << sformat(_("Command 'rollback' cannot be used on a non-root subvolume %s."),
			subvolume.c_str()) << endl;
	exit(EXIT_FAILURE);
    }

    pair<bool, unsigned int> previous_default = filesystem->getDefault();

    if (previous_default.first && scd1.description == "rollback backup")
	scd1.description += sformat(" of #%d", previous_default.second);

    ProxySnapshots& snapshots = snapper->getSnapshots();

    ProxySnapshots::const_iterator snapshot1 = snapshots.end();
    ProxySnapshots::const_iterator snapshot2 = snapshots.end();

    if (getopts.numArgs() == 0)
    {
	if (!quiet)
	    cout << _("Creating read-only snapshot of default subvolume.") << flush;

	scd1.read_only = true;
	snapshot1 = snapper->createSingleSnapshotOfDefault(scd1);

	if (!quiet)
	    cout << " " << sformat(_("(Snapshot %d.)"), snapshot1->getNum()) << endl;

	if (!quiet)
	    cout << _("Creating read-write snapshot of current subvolume.") << flush;

	scd2.read_only = false;
	snapshot2 = snapper->createSingleSnapshot(snapshots.getCurrent(), scd2);

	if (!quiet)
	    cout << " " << sformat(_("(Snapshot %d.)"), snapshot2->getNum()) << endl;
    }
    else
    {
	ProxySnapshots::const_iterator tmp = snapshots.findNum(getopts.popArg());

	if (!quiet)
	    cout << _("Creating read-only snapshot of current system.") << flush;

	snapshot1 = snapper->createSingleSnapshot(scd1);

	if (!quiet)
	    cout << " " << sformat(_("(Snapshot %d.)"), snapshot1->getNum()) << endl;

	if (!quiet)
	    cout << sformat(_("Creating read-write snapshot of snapshot %d."), tmp->getNum()) << flush;

	scd2.read_only = false;
	snapshot2 = snapper->createSingleSnapshot(tmp, scd2);

	if (!quiet)
	    cout << " " << sformat(_("(Snapshot %d.)"), snapshot2->getNum()) << endl;
    }

    if (previous_default.first)
    {
	ProxySnapshots::iterator it = snapshots.find(previous_default.second);
	if (it->getCleanup().empty())
	{
	    SMD smd = it->getSmd();
	    smd.cleanup = "number";
	    snapper->modifySnapshot(it, smd);
	}
    }

    if (!quiet)
	cout << sformat(_("Setting default subvolume to snapshot %d."), snapshot2->getNum()) << endl;

    filesystem->setDefault(snapshot2->getNum());

    Hooks::rollback(filesystem->snapshotDir(snapshot1->getNum()),
		    filesystem->snapshotDir(snapshot2->getNum()));

    if (print_number)
	cout << snapshot2->getNum() << endl;
}

#endif


void
help_setup_quota()
{
    cout << _("  Setup quota:") << endl
	 << _("\tsnapper setup-quota") << endl
	 << endl;
}


void
command_setup_quota(ProxySnappers* snappers, ProxySnapper* snapper)
{
    GetOpts::parsed_opts opts = getopts.parse("setup-quota", GetOpts::no_options);
    if (getopts.numArgs() != 0)
    {
	cerr << _("Command 'setup-quota' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    snapper->setupQuota();
}


void
help_cleanup()
{
    cout << _("  Cleanup snapshots:") << endl
	 << _("\tsnapper cleanup <cleanup-algorithm>") << endl
	 << endl;
}


void
command_cleanup(ProxySnappers* snappers, ProxySnapper* snapper)
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
	do_cleanup_number(snapper, verbose);
    }
    else if (cleanup == "timeline")
    {
	do_cleanup_timeline(snapper, verbose);
    }
    else if (cleanup == "empty-pre-post")
    {
	do_cleanup_empty_pre_post(snapper, verbose);
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
command_debug(ProxySnappers* snappers, ProxySnapper*)
{
    getopts.parse("debug", GetOpts::no_options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'debug' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    for (const string& line : snappers->debug())
	cout << line << endl;
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
    try
    {
        XAModification xa_mod = XAModification(XAttributes(loc_pre), XAttributes(loc_post));

        if (!xa_mod.empty())
	{
	    cout << "--- " << loc_pre << endl << "+++ " << loc_post << endl;
	    xa_mod.dumpDiffReport(cout);
	}
    }
    catch (const XAttributesException& e)
    {
	SN_CAUGHT(e);
    }
}

void
command_xa_diff(ProxySnappers* snappers, ProxySnapper* snapper)
{
    GetOpts::parsed_opts opts = getopts.parse("xadiff", GetOpts::no_options);
    if (getopts.numArgs() < 1)
    {
        cerr << _("Command 'xadiff' needs at least one argument.") << endl;
        exit(EXIT_FAILURE);
    }

    ProxySnapshots& snapshots = snapper->getSnapshots();

    pair<ProxySnapshots::const_iterator, ProxySnapshots::const_iterator> range =
	snapshots.findNums(getopts.popArg());

    ProxyComparison comparison = snapper->createComparison(*range.first, *range.second, true);

    MyFiles files(comparison.getFiles());

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
	 << _("\t--root, -r <path>\t\tOperate on target root (works only without DBus).") << endl
	 << _("\t--version\t\t\tPrint version and exit.") << endl
	 << endl;

    for (list<Cmd>::const_iterator cmd = cmds.begin(); cmd != cmds.end(); ++cmd)
	(*cmd->help_func)();

    exit (EXIT_SUCCESS);
}


int
main(int argc, char** argv)
{
    try
    {
	locale::global(locale(""));
    }
    catch (const runtime_error& e)
    {
	cerr << "Failed to set locale. Fix your system." << endl;
    }

    setLogDo(&log_do);
    setLogQuery(&log_query);

    const list<Cmd> cmds = {
	Cmd("list-configs", command_list_configs, help_list_configs, false),
	Cmd("create-config", command_create_config, help_create_config, false),
	Cmd("delete-config", command_delete_config, help_delete_config, false),
	Cmd("get-config", command_get_config, help_get_config, true),
	Cmd("set-config", command_set_config, help_set_config, true),
	Cmd("list", { "ls" }, command_list, help_list, false),
	Cmd("create", command_create, help_create, true),
	Cmd("modify", command_modify, help_modify, true),
	Cmd("delete", { "remove", "rm" }, command_delete, help_delete, true),
	Cmd("mount", command_mount, help_mount, true),
	Cmd("umount", command_umount, help_umount, true),
	Cmd("status", command_status, help_status, true),
	Cmd("diff", command_diff, help_diff, true),
#ifdef ENABLE_XATTRS
	Cmd("xadiff", command_xa_diff, help_xa_diff, true),
#endif
	Cmd("undochange", command_undo, help_undo, true),
#ifdef ENABLE_ROLLBACK
	Cmd("rollback", command_rollback, help_rollback, true),
#endif
	Cmd("setup-quota", command_setup_quota, help_setup_quota, true),
	Cmd("cleanup", command_cleanup, help_cleanup, true),
	Cmd("debug", command_debug, help_debug, false)
    };

    const struct option options[] = {
	{ "quiet",		no_argument,		0,	'q' },
	{ "verbose",		no_argument,		0,	'v' },
	{ "utc",		no_argument,		0,	0 },
	{ "iso",		no_argument,		0,	0 },
	{ "table-style",	required_argument,	0,	't' },
	{ "config",		required_argument,	0,	'c' },
	{ "no-dbus",		no_argument,		0,	0 },
	{ "root",		required_argument,	0,	'r' },
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

    if ((opt = opts.find("root")) != opts.end())
    {
	target_root = opt->second;
        if (!no_dbus)
        {
            cerr << _("root argument can be used only together with no-dbus.") << endl
                 << _("Try 'snapper --help' for more information.") << endl;
            exit(EXIT_FAILURE);
        }
    }

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

    try
    {
	ProxySnappers snappers(no_dbus ? ProxySnappers::createLib(target_root) :
			       ProxySnappers::createDbus());

	if (cmd->needs_snapper)
	    (*cmd->cmd_func)(&snappers, snappers.getSnapper(config_name));
	else
	    (*cmd->cmd_func)(&snappers, nullptr);
    }
    catch (const DBus::ErrorException& e)
    {
	SN_CAUGHT(e);

	if (strcmp(e.name(), "error.unknown_config") == 0 && config_name == "root")
	{
	    cerr << _("The config 'root' does not exist. Likely snapper is not configured.") << endl
		 << _("See 'man snapper' for further instructions.") << endl;
	    exit(EXIT_FAILURE);
	}

	cerr << error_description(e) << endl;
	exit(EXIT_FAILURE);
    }
    catch (const DBus::FatalException& e)
    {
	SN_CAUGHT(e);
	cerr << _("Failure") << " (" << e.what() << ")." << endl;
	exit(EXIT_FAILURE);
    }
    catch (const IllegalSnapshotException& e)
    {
	SN_CAUGHT(e);
	cerr << _("Illegal snapshot.") << endl;
	exit(EXIT_FAILURE);
    }
    catch (const ConfigNotFoundException& e)
    {
	SN_CAUGHT(e);
	cerr << sformat(_("Config '%s' not found."), config_name.c_str()) << endl;
	exit(EXIT_FAILURE);
    }
    catch (const InvalidConfigException& e)
    {
	SN_CAUGHT(e);
	cerr << sformat(_("Config '%s' is invalid."), config_name.c_str()) << endl;
	exit(EXIT_FAILURE);
    }
    catch (const ListConfigsFailedException& e)
    {
	SN_CAUGHT(e);
	cerr << sformat(_("Listing configs failed (%s)."), e.what()) << endl;
	exit(EXIT_FAILURE);
    }
    catch (const CreateConfigFailedException& e)
    {
	SN_CAUGHT(e);
	cerr << sformat(_("Creating config failed (%s)."), e.what()) << endl;
	exit(EXIT_FAILURE);
    }
    catch (const DeleteConfigFailedException& e)
    {
	SN_CAUGHT(e);
	cerr << sformat(_("Deleting config failed (%s)."), e.what()) << endl;
	exit(EXIT_FAILURE);
    }
    catch (const InvalidConfigdataException& e)
    {
	SN_CAUGHT(e);
	cerr << _("Invalid configdata.") << endl;
	exit(EXIT_FAILURE);
    }
    catch (const AclException& e)
    {
	SN_CAUGHT(e);
	cerr << _("ACL error.") << endl;
	exit(EXIT_FAILURE);
    }
    catch (const IOErrorException& e)
    {
	SN_CAUGHT(e);
	cerr << sformat(_("IO error (%s)."), e.what()) << endl;
	exit(EXIT_FAILURE);
    }
    catch (const InvalidUserException& e)
    {
	SN_CAUGHT(e);
	cerr << _("Invalid user.") << endl;
	exit(EXIT_FAILURE);
    }
    catch (const InvalidGroupException& e)
    {
	SN_CAUGHT(e);
	cerr << _("Invalid group.") << endl;
	exit(EXIT_FAILURE);
    }
    catch (const QuotaException& e)
    {
	SN_CAUGHT(e);
	cerr << sformat(_("Quota error (%s)."), e.what()) << endl;
	exit(EXIT_FAILURE);
    }
    catch (const Exception& e)
    {
	SN_CAUGHT(e);
	cerr << sformat(_("Error (%s)."), e.what()) << endl;
	exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
