
#include <stdlib.h>
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
	cerr << sformat(_("Snapshot '%u' not found"), num) << endl;
	exit(EXIT_FAILURE);
    }

    return snap;
}


void
showHelp()
{
    getopts.parse("help", GetOpts::no_options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'help' does not take arguments") << endl;
	exit(EXIT_FAILURE);
    }

    cout <<
	"Usage: snapper [--global-opts] <command> [--command-opts] [command-arguments]\n"
	"\n"
	"Possible commands are:\n"
	"    help                 -- show this help\n"
	"    list                 -- list all snapshots\n"
	"    create single [desc] -- create single snapshot with \"descr\" as description\n"
	"    create pre [desc]    -- create pre snapshot with \"descr\" as description\n"
	"    create post num      -- create post snapshot that corresponds to\n"
	"                            pre snapshot number \"num\"\n"
	"    diff num1 num2       -- show difference between snapnots num1 and num2.\n"
	"                            current version of filesystem is indicated by number 0.\n"
	"\n";
}


void
listSnap()
{
    getopts.parse("list", GetOpts::no_options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'list' does not take arguments") << endl;
	exit(EXIT_FAILURE);
    }

    Table table;

    TableHeader header;
    header.add("Type");
    header.add("#");
    header.add("Pre #");
    header.add("Date");
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
	row.add(it->getDescription());
	table.add(row);
    }

    cout << table;
}


void
createSnap()
{
    const struct option options[] = {
	{ "type",		required_argument,	0,	't' },
	{ "pre-number",		required_argument,	0,	0 },
	{ "description",	required_argument,	0,	'd' },
	{ "print-number",	no_argument,		0,	'p' },
	{ 0, 0, 0, 0 }
    };

    GetOpts::parsed_opts opts = getopts.parse("create", options);
    if (getopts.hasArgs())
    {
	cerr << _("Command 'create' does not take arguments") << endl;
	exit(EXIT_FAILURE);
    }

    SnapshotType type = SINGLE;
    Snapshots::const_iterator snap1;
    string description;
    bool print_number = false;

    GetOpts::parsed_opts::const_iterator it;

    if ((it = opts.find("type")) != opts.end())
	toValue(it->second, type, SINGLE);

    if ((it = opts.find("pre-number")) != opts.end())
	snap1 = readNum(it->second);

    if ((it = opts.find("description")) != opts.end())
	description = it->second;

    if ((it = opts.find("print-number")) != opts.end())
	print_number = true;

    switch (type)
    {
	case SINGLE: {
	    Snapshots::const_iterator snap1 = sh->createSingleSnapshot(description);
	    if (print_number)
		cout << snap1->getNum() << endl;
	} break;

	case PRE: {
	    Snapshots::const_iterator snap1 = sh->createPreSnapshot(description);
	    if (print_number)
		cout << snap1->getNum() << endl;
	} break;

	case POST: {
	    Snapshots::const_iterator snap2 = sh->createPostSnapshot(snap1);
	    if (print_number)
		cout << snap2->getNum() << endl;
	    sh->startBackgroundComparsion(snap1, snap2);
	} break;
    }
}


void
deleteSnap()
{
    getopts.parse("delete", GetOpts::no_options);
    if (!getopts.hasArgs())
    {
	cerr << _("Command 'delete' needs at least one argument") << endl;
	exit(EXIT_FAILURE);
    }

    while (getopts.hasArgs())
    {
	Snapshots::iterator snapshot = readNum(getopts.popArg());
	sh->deleteSnapshot(snapshot);
    }
}


void
showDifference()
{
    getopts.parse("diff", GetOpts::no_options);
    if (getopts.numArgs() != 2)
    {
	cerr << _("Command 'diff' needs two arguments") << endl;
	exit(EXIT_FAILURE);
    }

    Snapshots::const_iterator snap1 = readNum(getopts.popArg());
    Snapshots::const_iterator snap2 = readNum(getopts.popArg());

    sh->setComparison(snap1, snap2);

    const Files& files = sh->getFiles();
    for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	cout << statusToString(it->getPreToPostStatus()) << " " << it->getName() << endl;
}


void
doRollback()
{
    getopts.parse("rollback", GetOpts::no_options);
    if (getopts.numArgs() != 2)
    {
	cerr << _("Command 'rollback' needs two arguments") << endl;
	exit(EXIT_FAILURE);
    }

    Snapshots::const_iterator snap1 = readNum(getopts.popArg());
    Snapshots::const_iterator snap2 = readNum(getopts.popArg());

    sh->setComparison(snap1, snap2);

    Files& files = sh->getFiles();
    for (Files::iterator it = files.begin(); it != files.end(); ++it)
	it->setRollback(true);

    sh->doRollback();
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

    cmds["help"] = showHelp;
    cmds["list"] = listSnap;
    cmds["create"] = createSnap;
    cmds["delete"] = deleteSnap;
    cmds["diff"] = showDifference;
    cmds["rollback"] = doRollback;

    const struct option options[] = {
	{ "quiet",		no_argument,		0,	'q' },
	{ "table-style",	required_argument,	0,	's' },
	{ 0, 0, 0, 0 }
    };

    getopts.init(argc, argv);

    GetOpts::parsed_opts opts = getopts.parse(options);
    if (!getopts.hasArgs())
    {
	cerr << _("No command provided") << endl;
	exit(EXIT_FAILURE);
    }

    const char* command = getopts.popArg();
    map<string, cmd_fnc>::const_iterator cmd = cmds.find(command);
    if (cmd == cmds.end())
    {
	cerr << sformat(_("Unknown command '%s'"), command) << endl;
	exit(EXIT_FAILURE);
    }

    GetOpts::parsed_opts::const_iterator it;
    if ((it = opts.find("quiet")) != opts.end())
	quiet = true;

    if ((it = opts.find("table-style")) != opts.end())
    {
	unsigned int s;
	it->second >> s;
	if (s >= _End)
	{
	    cerr << sformat(_("Invalid table style %d."), s) << " "
		 << sformat(_("Use an integer number from %d to %d"), 0, _End - 1) << endl;
	    exit(EXIT_FAILURE);
	}
	Table::defaultStyle = (TableLineStyle) s;
    }

    sh = createSnapper();

    if (!quiet)
	sh->setCompareCallback(&compare_callback_impl);

    (*cmd->second)();

    deleteSnapper(sh);

    exit(EXIT_SUCCESS);
}
