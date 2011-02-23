
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include <snapper/Factory.h>
#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/File.h>
#include <snapper/AppUtil.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/Compare.h>
#include <snapper/Enum.h>

#include "utils/Table.h"
#include "utils/GetOpts.h"

using namespace snapper;
using namespace std;


typedef void (*cmd_fnc)();
map<string, cmd_fnc> cmds;

GetOpts getopts;

bool quiet = false;
string root = "/";

Snapper* sh = NULL;


Snapshots::iterator
readNum(const string& str)
{
    Snapshots& snapshots = sh->getSnapshots();

    unsigned int num = 0;
    if (str != "current")
	str >> num;

    Snapshots::iterator snap = snapshots.find(num);
    if (snap == snapshots.end())
    {
	cerr << sformat(_("Snapshot '%u' not found."), num) << endl;
	exit(EXIT_FAILURE);
    }

    return snap;
}


void
help_list()
{
    cout << _("  List snapshots:") << endl
	 << _("\tsnapper list") << endl
	 << endl;
}


void
command_list()
{
    getopts.parse("list", GetOpts::no_options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'list' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    Table table;

    TableHeader header;
    header.add("Type");
    header.add("#");
    header.add("Pre #");
    header.add("Date");
    header.add("Cleanup");
    header.add("Description");
    table.setHeader(header);

    const Snapshots& snapshots = sh->getSnapshots();
    for (Snapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
    {
	TableRow row;
	row.add(toString(it->getType()));
	row.add(decString(it->getNum()));
	row.add(it->getType() == POST ? decString(it->getPreNum()) : "");
	row.add(it->isCurrent() ? "" : datetime(it->getDate(), false, false));
	row.add(it->getCleanup());
	row.add(it->getDescription());
	table.add(row);
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
	 << _("\t--description, -d <description>\tDescription for snapshot.") << endl
	 << _("\t--print-number, -p\t\tPrint number of created snapshot.") << endl
	 << _("\t--cleanup-algorithm, -c\t\tCleanup algorithm for snapshot.") << endl
	 << endl;
}


void
command_create()
{
    const struct option options[] = {
	{ "type",		required_argument,	0,	't' },
	{ "pre-number",		required_argument,	0,	0 },
	{ "description",	required_argument,	0,	'd' },
	{ "print-number",	no_argument,		0,	'p' },
	{ "cleanup",		required_argument,	0,	'c' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("create", options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'create' does not take arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    SnapshotType type = SINGLE;
    Snapshots::const_iterator snap1;
    string description;
    bool print_number = false;
    string cleanup;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("type")) != opts.end())
	toValue(opt->second, type, SINGLE);

    if ((opt = opts.find("pre-number")) != opts.end())
	snap1 = readNum(opt->second);

    if ((opt = opts.find("description")) != opts.end())
	description = opt->second;

    if ((opt = opts.find("print-number")) != opts.end())
	print_number = true;

    if ((opt = opts.find("cleanup")) != opts.end())
	cleanup = opt->second;

    switch (type)
    {
	case SINGLE: {
	    Snapshots::iterator snap1 = sh->createSingleSnapshot(description);
	    snap1->setCleanup(cleanup);
	    if (print_number)
		cout << snap1->getNum() << endl;
	} break;

	case PRE: {
	    Snapshots::iterator snap1 = sh->createPreSnapshot(description);
	    snap1->setCleanup(cleanup);
	    if (print_number)
		cout << snap1->getNum() << endl;
	} break;

	case POST: {
	    Snapshots::iterator snap2 = sh->createPostSnapshot(snap1);
	    snap2->setCleanup(cleanup);
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
	 << endl;
}


void
command_modify()
{
    const struct option options[] = {
	{ "description",	required_argument,	0,	'd' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("modify", options);
    if (getopts.numArgs() != 1)
    {
	cerr << _("Command 'modify' need one argument.") << endl;
	exit(EXIT_FAILURE);
    }

    Snapshots::iterator snapshot = readNum(getopts.popArg());

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("description")) != opts.end())
	snapshot->setDescription(opt->second);
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
	Snapshots::iterator snapshot = readNum(getopts.popArg());
	sh->deleteSnapshot(snapshot);
    }
}


void
help_diff()
{
    cout << _("  Comparing snapshots:") << endl
	 << _("\tsnapper diff <number1> <number2>") << endl
	 << endl
	 << _("    Options for 'diff' command:") << endl
	 << _("\t--output, -o <file>\t\tSave diff to file.") << endl
	 << _("\t--file, -f <file>\t\tRun diff for file.") << endl
	 << endl;
}


void
command_diff()
{
    const struct option options[] = {
	{ "output",		required_argument,	0,	'o' },
	{ "file",		required_argument,	0,	'f' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("diff", options);
    if (getopts.numArgs() != 2)
    {
	cerr << _("Command 'diff' needs two arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    GetOpts::parsed_opts::const_iterator opt;

    Snapshots::const_iterator snap1 = readNum(getopts.popArg());
    Snapshots::const_iterator snap2 = readNum(getopts.popArg());

    sh->setComparison(snap1, snap2);

    const Files& files = sh->getFiles();

    Files::const_iterator tmp = files.end();

    if ((opt = opts.find("file")) != opts.end())
    {
	tmp = files.find(opt->second);
	if (tmp == files.end())
	{
	    cerr << sformat(_("File '%s' not included in diff."), opt->second.c_str()) << endl;
	    exit(EXIT_FAILURE);
	}
    }

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

    if (tmp == files.end())
    {
	for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	    fprintf(file, "%s %s\n", statusToString(it->getPreToPostStatus()).c_str(), it->getName().c_str());
    }
    else
    {
	vector<string> lines = tmp->getDiff("-u");
	for (vector<string>::const_iterator it = lines.begin(); it != lines.end(); ++it)
	    fprintf(file, "%s\n", it->c_str());
    }

    if (file != stdout)
	fclose(file);
}


void
help_rollback()
{
    cout << _("  Rollback snapshots:") << endl
	 << _("\tsnapper rollback <number1> <number2>") << endl
	 << endl
	 << _("    Options for 'rollback' command:") << endl
	 << _("\t--file, -f <file>\t\tRead files to rollback from file.") << endl
	 << endl;
}


void
command_rollback()
{
    const struct option options[] = {
	{ "file",		required_argument,	0,	'f' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("rollback", options);
    if (getopts.numArgs() != 2)
    {
	cerr << _("Command 'rollback' needs two arguments.") << endl;
	exit(EXIT_FAILURE);
    }

    Snapshots::const_iterator snap1 = readNum(getopts.popArg());
    Snapshots::const_iterator snap2 = readNum(getopts.popArg());

    FILE* file = NULL;

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("file")) != opts.end())
    {
	file = fopen(opt->second.c_str(), "r");
	if (!file)
	{
	    cerr << sformat(_("Opening file '%s' failed."), opt->second.c_str()) << endl;
	    exit(EXIT_FAILURE);
	}
    }

    sh->setComparison(snap1, snap2);

    Files& files = sh->getFiles();

    if (file)
    {
	char* line = NULL;
	size_t len = 0;

	while (getline(&line, &len, file) != -1)
	{
	    // TODO: more robust splitting, make status optional

	    string name = string(line, 5, strlen(line) - 6);

	    Files::iterator it = files.find(name);
	    if (it == files.end())
	    {
		cerr << sformat(_("File '%s' not found in diff."), name.c_str()) << endl;
		exit(EXIT_FAILURE);
	    }

	    it->setRollback(true);
	}

	free(line);

	fclose(file);
    }
    else
    {
	for (Files::iterator it = files.begin(); it != files.end(); ++it)
	    it->setRollback(true);
    }

    RollbackStatistic rs = sh->getRollbackStatistic();

    if (rs.empty())
    {
	cout << "nothing to do" << endl;
	return;
    }

    cout << "create:" << rs.numCreate << " modify:" << rs.numModify << " delete:" << rs.numDelete
	 << endl;

    sh->doRollback();
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

    if (cleanup == "amount")
    {
	sh->doCleanupAmount();
    }
    else if (cleanup == "timeline")
    {
	sh->doCleanupTimeline();
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
	 << _("\t--table-style, -s <style>\tTable style (integer).") << endl
	 << _("\t--root, -r <path>\t\tSet root directory.") << endl
	 << endl;

    help_list();
    help_create();
    help_modify();
    help_delete();
    help_diff();
    help_rollback();
    help_cleanup();
}


struct CompareCallbackImpl : public CompareCallback
{
    void start() {  cout << "comparing snapshots..." << flush; }
    void stop() { cout << " done" << endl; }
};

CompareCallbackImpl compare_callback_impl;


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    initDefaultLogger();

    cmds["list"] = command_list;
    cmds["create"] = command_create;
    cmds["modify"] = command_modify;
    cmds["delete"] = command_delete;
    cmds["diff"] = command_diff;
    cmds["rollback"] = command_rollback;
    cmds["cleanup"] = command_cleanup;
    cmds["help"] = command_help;

    const struct option options[] = {
	{ "quiet",		no_argument,		0,	'q' },
	{ "table-style",	required_argument,	0,	's' },
	{ "root",		required_argument,	0,	'r' },
	{ 0, 0, 0, 0 }
    };

    getopts.init(argc, argv);

    GetOpts::parsed_opts opts = getopts.parse(options);
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

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("quiet")) != opts.end())
	quiet = true;

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

    if ((opt = opts.find("root")) != opts.end())
	root = opt->second;

    sh = createSnapper(root);

    if (!quiet)
	sh->setCompareCallback(&compare_callback_impl);

    (*cmd->second)();

    deleteSnapper(sh);

    exit(EXIT_SUCCESS);
}
