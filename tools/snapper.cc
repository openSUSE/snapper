/*
 * Copyright (c) 2011 Novell, Inc.
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


#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <boost/algorithm/string.hpp>

#include "config.h"
#include <snapper/Factory.h>
#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/Comparison.h>
#include <snapper/File.h>
#include <snapper/AppUtil.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/Compare.h>
#include <snapper/Enum.h>
#include <snapper/AsciiFile.h>

#include "utils/Table.h"
#include "utils/GetOpts.h"

using namespace snapper;
using namespace std;


typedef void (*cmd_fnc)();
map<string, cmd_fnc> cmds;

GetOpts getopts;

bool quiet = false;
bool verbose = false;
string config_name = "root";
bool disable_filters = false;

Snapper* sh = NULL;


void
help_list_configs()
{
    cout << _("  List configs:") << endl
	 << _("\tsnapper list-configs") << endl
	 << endl;
}


void
command_list_configs()
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

    try
    {
	list<ConfigInfo> config_infos = Snapper::getConfigs();
	for (list<ConfigInfo>::const_iterator it = config_infos.begin(); it != config_infos.end(); ++it)
	{
	    TableRow row;
	    row.add(it->config_name);
	    row.add(it->subvolume);
	    table.add(row);
	}
    }
    catch (const ListConfigsFailedException& e)
    {
	cerr << sformat(_("Listing configs failed (%s)."), e.what()) << endl;
	exit(EXIT_FAILURE);
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
command_create_config()
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

    try
    {
	Snapper::addConfig(config_name, subvolume, fstype, template_name);
    }
    catch (const AddConfigFailedException& e)
    {
	cerr << sformat(_("Creating config failed (%s)."), e.what()) << endl;
	exit(EXIT_FAILURE);
    }
}


Snapshots::iterator
read_num(const string& str)
{
    Snapshots& snapshots = sh->getSnapshots();

    istringstream s(str);
    unsigned int num = 0;
    s >> num;

    if (s.fail() || !s.eof())
    {
	cerr << sformat(_("Invalid snapshot '%s'."), str.c_str()) << endl;
	exit(EXIT_FAILURE);
    }

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
    {
	cerr << sformat(_("Snapshot '%u' not found."), num) << endl;
	exit(EXIT_FAILURE);
    }

    return snap;
}


pair<Snapshots::iterator, Snapshots::iterator>
read_nums(const string& str)
{
    string::size_type pos = str.find("..");
    if (pos == string::npos)
    {
	cerr << _("Invalid snapshots.") << endl;
	exit(EXIT_FAILURE);
    }

    Snapshots::iterator snap1 = read_num(str.substr(0, pos));
    Snapshots::iterator snap2 = read_num(str.substr(pos + 2));

    if (snap1 == snap2)
    {
	cerr << _("Identical snapshots.") << endl;
	exit(EXIT_FAILURE);
    }

    return pair<Snapshots::iterator, Snapshots::iterator>(snap1, snap2);
}


map<string, string>
read_userdata(const string& s, const map<string, string>& old = map<string, string>())
{
    map<string, string> userdata = old;

    list<string> tmp;
    boost::split(tmp, s, boost::is_any_of(","), boost::token_compress_on);
    if (tmp.empty())
    {
	cerr << _("Invalid userdata.") << endl;
	exit(EXIT_FAILURE);
    }

    for (list<string>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
	string::size_type pos = it->find("=");
	if (pos == string::npos)
	{
	    cerr << _("Invalid userdata.") << endl;
	    exit(EXIT_FAILURE);
	}

	string key = boost::trim_copy(it->substr(0, pos));
	string value = boost::trim_copy(it->substr(pos + 1));

	if (key.empty())
	{
	    cerr << _("Invalid userdata.") << endl;
	    exit(EXIT_FAILURE);
	}

	if (value.empty())
	    userdata.erase(key);
	else
	    userdata[key] = value;
    }

    return userdata;
}


string
show_userdata(const map<string, string>& userdata)
{
    string s;

    for (map<string, string>::const_iterator it = userdata.begin(); it != userdata.end(); ++it)
    {
	if (!s.empty())
	    s += ", ";
	s += it->first + "=" + it->second;
    }

    return s;
}


void
help_list()
{
    cout << _("  List snapshots:") << endl
	 << _("\tsnapper list") << endl
	 << endl
	 << _("    Options for 'list' command:") << endl
	 << _("\t--type, -t <type>\t\tType of snapshots to list.") << endl
	 << endl;
}


void
command_list()
{
    const struct option options[] = {
	{ "type",		required_argument,	0,	't' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("list", options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'list' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    enum ListMode { LM_ALL, LM_SINGLE, LM_PRE_POST };
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
	    header.add(_("Cleanup"));
	    header.add(_("Description"));
	    header.add(_("Userdata"));
	    table.setHeader(header);

	    const Snapshots& snapshots = sh->getSnapshots();
	    for (Snapshots::const_iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
	    {
		TableRow row;
		row.add(toString(it1->getType()));
		row.add(decString(it1->getNum()));
		row.add(it1->getType() == POST ? decString(it1->getPreNum()) : "");
		row.add(it1->isCurrent() ? "" : datetime(it1->getDate(), false, false));
		row.add(it1->getCleanup());
		row.add(it1->getDescription());
		row.add(show_userdata(it1->getUserdata()));
		table.add(row);
	    }
	}
	break;

	case LM_SINGLE:
	{
	    TableHeader header;
	    header.add(_("#"));
	    header.add(_("Date"));
	    header.add(_("Description"));
	    header.add(_("Userdata"));
	    table.setHeader(header);

	    const Snapshots& snapshots = sh->getSnapshots();
	    for (Snapshots::const_iterator it1 = snapshots.begin(); it1 != snapshots.end(); ++it1)
	    {
		if (it1->getType() != SINGLE)
		    continue;

		TableRow row;
		row.add(decString(it1->getNum()));
		row.add(it1->isCurrent() ? "" : datetime(it1->getDate(), false, false));
		row.add(it1->getDescription());
		row.add(show_userdata(it1->getUserdata()));
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

	    const Snapshots& snapshots = sh->getSnapshots();
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
		row.add(datetime(it1->getDate(), false, false));
		row.add(datetime(it2->getDate(), false, false));
		row.add(it1->getDescription());
		row.add(show_userdata(it1->getUserdata()));
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
	 << endl;
}


void
command_create()
{
    const struct option options[] = {
	{ "type",		required_argument,	0,	't' },
	{ "pre-number",		required_argument,	0,	0 },
	{ "print-number",	no_argument,		0,	'p' },
	{ "description",	required_argument,	0,	'd' },
	{ "cleanup-algorithm",	required_argument,	0,	'c' },
	{ "userdata",		required_argument,	0,	'u' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("create", options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'create' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    const Snapshots& snapshots = sh->getSnapshots();

    SnapshotType type = SINGLE;
    Snapshots::const_iterator snap1 = snapshots.end();
    bool print_number = false;
    string description;
    string cleanup;
    map<string, string> userdata;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("type")) != opts.end())
    {
	if (!toValue(opt->second, type, SINGLE))
	{
	    cerr << _("Unknown type of snapshot.") << endl;
	    exit(EXIT_FAILURE);
	}
    }

    if ((opt = opts.find("pre-number")) != opts.end())
	snap1 = read_num(opt->second);

    if ((opt = opts.find("print-number")) != opts.end())
	print_number = true;

    if ((opt = opts.find("description")) != opts.end())
	description = opt->second;

    if ((opt = opts.find("cleanup-algorithm")) != opts.end())
	cleanup = opt->second;

    if ((opt = opts.find("userdata")) != opts.end())
	userdata = read_userdata(opt->second);

    if (type == POST && (snap1 == snapshots.end() || snap1->isCurrent()))
    {
	cerr << _("Missing or invalid pre-number.") << endl;
	exit(EXIT_FAILURE);
    }

    switch (type)
    {
	case SINGLE: {
	    Snapshots::iterator snap1 = sh->createSingleSnapshot(description);
	    snap1->setCleanup(cleanup);
	    snap1->setUserdata(userdata);
	    snap1->flushInfo();
	    if (print_number)
		cout << snap1->getNum() << endl;
	} break;

	case PRE: {
	    Snapshots::iterator snap1 = sh->createPreSnapshot(description);
	    snap1->setCleanup(cleanup);
	    snap1->setUserdata(userdata);
	    snap1->flushInfo();
	    if (print_number)
		cout << snap1->getNum() << endl;
	} break;

	case POST: {
	    Snapshots::iterator snap2 = sh->createPostSnapshot(snap1);
	    snap2->setCleanup(cleanup);
	    snap2->setUserdata(userdata);
	    snap2->flushInfo();
	    if (print_number)
		cout << snap2->getNum() << endl;
	    sh->startBackgroundComparsion(snap1, snap2);
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
command_modify()
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
	Snapshots::iterator snapshot = read_num(getopts.popArg());
	if (snapshot->isCurrent())
	{
	    cerr << _("Invalid snapshot.") << endl;
	    exit(EXIT_FAILURE);
	}

	GetOpts::parsed_opts::const_iterator opt;

	if ((opt = opts.find("description")) != opts.end())
	    snapshot->setDescription(opt->second);

	if ((opt = opts.find("cleanup-algorithm")) != opts.end())
	    snapshot->setCleanup(opt->second);

	if ((opt = opts.find("userdata")) != opts.end())
	    snapshot->setUserdata(read_userdata(opt->second, snapshot->getUserdata()));

	snapshot->flushInfo();
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
command_delete()
{
    getopts.parse("delete", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'delete' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    while (getopts.hasArgs())
    {
	Snapshots::iterator snapshot = read_num(getopts.popArg());
	if (snapshot->isCurrent())
	{
	    cerr << _("Invalid snapshot.") << endl;
	    exit(EXIT_FAILURE);
	}

	sh->deleteSnapshot(snapshot);
    }
}


void
help_mount()
{
    cout << _("  Mount snapshot:") << endl
	 << _("\tsnapper mount <number>") << endl
	 << endl;
}


void
command_mount()
{
    getopts.parse("mount", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'mount' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    while (getopts.hasArgs())
    {
	Snapshots::iterator snapshot = read_num(getopts.popArg());
	if (snapshot->isCurrent())
	{
	    cerr << _("Invalid snapshot.") << endl;
	    exit(EXIT_FAILURE);
	}

	snapshot->mountFilesystemSnapshot();
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
command_umount()
{
    getopts.parse("mount", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'mount' needs at least one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    while (getopts.hasArgs())
    {
	Snapshots::iterator snapshot = read_num(getopts.popArg());
	if (snapshot->isCurrent())
	{
	    cerr << _("Invalid snapshot.") << endl;
	    exit(EXIT_FAILURE);
	}

	snapshot->umountFilesystemSnapshot();
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
command_status()
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

    pair<Snapshots::const_iterator, Snapshots::const_iterator> snaps(read_nums(getopts.popArg()));

    Comparison comparison(sh, snaps.first, snaps.second);

    const Files& files = comparison.getFiles();

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
	 << endl;
}


void
command_diff()
{
    GetOpts::parsed_opts opts = getopts.parse("diff", GetOpts::no_options);

    GetOpts::parsed_opts::const_iterator opt;

    pair<Snapshots::const_iterator, Snapshots::const_iterator> snaps(read_nums(getopts.popArg()));

    Comparison comparison(sh, snaps.first, snaps.second);

    const Files& files = comparison.getFiles();

    if (getopts.numArgs() == 0)
    {
	for (Files::const_iterator it1 = files.begin(); it1 != files.end(); ++it1)
	{
	    vector<string> lines = it1->getDiff("--unified --new-file");
	    for (vector<string>::const_iterator it2 = lines.begin(); it2 != lines.end(); ++it2)
		cout << it2->c_str() << endl;
	}
    }
    else
    {
	while (getopts.numArgs() > 0)
	{
	    string name = getopts.popArg();

	    Files::const_iterator tmp = files.findAbsolutePath(name);
	    if (tmp == files.end())
		continue;

	    vector<string> lines = tmp->getDiff("--unified --new-file");
	    for (vector<string>::const_iterator it2 = lines.begin(); it2 != lines.end(); ++it2)
		cout << it2->c_str() << endl;
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
command_undo()
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

    pair<Snapshots::const_iterator, Snapshots::const_iterator> snaps(read_nums(getopts.popArg()));

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

    if (snaps.first->isCurrent())
    {
	cerr << _("Invalid snapshots.") << endl;
	exit(EXIT_FAILURE);
    }

    Comparison comparison(sh, snaps.first, snaps.second);

    Files& files = comparison.getFiles();

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
		string name = getopts.popArg();

		Files::iterator tmp = files.findAbsolutePath(name);
		if (tmp == files.end())
		    continue;

		tmp->setUndo(true);
	    }
	}
    }

    UndoStatistic rs = comparison.getUndoStatistic();

    if (rs.empty())
    {
	cout << "nothing to do" << endl;
	return;
    }

    cout << "create:" << rs.numCreate << " modify:" << rs.numModify << " delete:" << rs.numDelete
	 << endl;

    comparison.doUndo();
}


void
help_cleanup()
{
    cout << _("  Cleanup snapshots:") << endl
	 << _("\tsnapper cleanup <cleanup-algorithm>") << endl
	 << endl;
}


void
command_cleanup()
{
    const struct option options[] = {
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("cleanup", options);
    if (getopts.numArgs() != 1)
    {
	cerr << _("Command 'cleanup' needs one arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    string cleanup = getopts.popArg();

    if (cleanup == "number")
    {
	sh->doCleanupNumber();
    }
    else if (cleanup == "timeline")
    {
	sh->doCleanupTimeline();
    }
    else if (cleanup == "empty-pre-post")
    {
	sh->doCleanupEmptyPrePost();
    }
    else
    {
	cerr << sformat(_("Unknown cleanup algorithm '%s'."), cleanup.c_str()) << endl;
	exit(EXIT_FAILURE);
    }
}


void
command_help()
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
	 << _("\t--table-style, -t <style>\tTable style (integer).") << endl
	 << _("\t--config, -c <name>\t\tSet name of config to use.") << endl
	 << _("\t--disable-filters\t\tDisable filters.") << endl
	 << _("\t--version\t\t\tPrint version and exit.") << endl
	 << endl;

    help_list_configs();
    help_create_config();
    help_list();
    help_create();
    help_modify();
    help_delete();
    help_mount();
    help_umount();
    help_status();
    help_diff();
    help_undo();
    help_cleanup();
}


struct CompareCallbackImpl : public CompareCallback
{
    void start() { cout << _("comparing snapshots...") << flush; }
    void stop() { cout << " " << _("done") << endl; }
};

CompareCallbackImpl compare_callback_impl;


struct UndoCallbackImpl : public UndoCallback
{
    void start() { cout << _("undoing change...") << endl; }
    void stop() { cout << _("undoing change done") << endl; }

    void createInfo(const string& name)
	{ if (verbose) cout << sformat(_("creating %s"), name.c_str()) << endl; }
    void modifyInfo(const string& name)
	{ if (verbose) cout << sformat(_("modifying %s"), name.c_str()) << endl; }
    void deleteInfo(const string& name)
	{ if (verbose) cout << sformat(_("deleting %s"), name.c_str()) << endl; }

    void createError(const string& name)
	{ cerr << sformat(_("failed to create %s"), name.c_str()) << endl; }
    void modifyError(const string& name)
	{ cerr << sformat(_("failed to modify %s"), name.c_str()) << endl; }
    void deleteError(const string& name)
	{ cerr << sformat(_("failed to delete %s"), name.c_str()) << endl; }
};

UndoCallbackImpl undo_callback_impl;


int
main(int argc, char** argv)
{
    umask(0027);

    setlocale(LC_ALL, "");

    initDefaultLogger();

    cmds["list-configs"] = command_list_configs;
    cmds["create-config"] = command_create_config;
    cmds["list"] = command_list;
    cmds["create"] = command_create;
    cmds["modify"] = command_modify;
    cmds["delete"] = command_delete;
    cmds["mount"] = command_mount;
    cmds["umount"] = command_umount;
    cmds["status"] = command_status;
    cmds["diff"] = command_diff;
    cmds["undochange"] = command_undo;
    cmds["cleanup"] = command_cleanup;
    cmds["help"] = command_help;

    const struct option options[] = {
	{ "quiet",		no_argument,		0,	'q' },
	{ "verbose",		no_argument,		0,	'v' },
	{ "table-style",	required_argument,	0,	't' },
	{ "config",		required_argument,	0,	'c' },
	{ "disable-filters",	no_argument,		0,	0 },
	{ "version",		no_argument,		0,	0 },
	{ 0, 0, 0, 0 }
    };

    getopts.init(argc, argv);

    GetOpts::parsed_opts opts = getopts.parse(options);

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("quiet")) != opts.end())
	quiet = true;

    if ((opt = opts.find("verbose")) != opts.end())
	verbose = true;

    if ((opt = opts.find("table-style")) != opts.end())
    {
	unsigned int s;
	opt->second >> s;
	if (s >= _End)
	{
	    cerr << sformat(_("Invalid table style %d."), s) << " "
		 << sformat(_("Use an integer number from %d to %d"), 0, _End - 1) << endl;
	    exit(EXIT_FAILURE);
	}
	Table::defaultStyle = (TableLineStyle) s;
    }

    if ((opt = opts.find("config")) != opts.end())
	config_name = opt->second;

    if ((opt = opts.find("disable-filters")) != opts.end())
	disable_filters = true;

    if ((opt = opts.find("version")) != opts.end())
    {
	cout << "snapper " << VERSION << endl;
	exit(EXIT_SUCCESS);
    }

    if (!getopts.hasArgs())
    {
	cerr << _("No command provided.") << endl
	     << _("Try 'snapper help' for more information.") << endl;
	exit(EXIT_FAILURE);
    }

    const char* command = getopts.popArg();
    map<string, cmd_fnc>::const_iterator cmd = cmds.find(command);
    if (cmd == cmds.end())
    {
	cerr << sformat(_("Unknown command '%s'."), command) << endl
	     << _("Try 'snapper help' for more information.") << endl;
	exit(EXIT_FAILURE);
    }

    if (cmd->first == "help" || cmd->first == "list-configs" || cmd->first == "create-config")
    {
	(*cmd->second)();
    }
    else
    {
	try
	{
	    sh = createSnapper(config_name, disable_filters);
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

	if (!quiet)
	{
	    sh->setCompareCallback(&compare_callback_impl);
	    sh->setUndoCallback(&undo_callback_impl);
	}

	try
	{
	    (*cmd->second)();
	}
	catch (const SnapperException& e)
	{
	    y2err("caught final exception");
	    cerr << sformat(_("Command failed (%s). See log for more information."),
			    e.what()) << endl;
	    exit(EXIT_FAILURE);
	}

	deleteSnapper(sh);
    }

    exit(EXIT_SUCCESS);
}
