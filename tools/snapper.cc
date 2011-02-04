
#include <stdlib.h>
#include <getopt.h>
#include <iostream>

#include <snapper/Factory.h>
#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/File.h>
#include <snapper/AppUtil.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/Compare.h>
#include <snapper/Enum.h>

using namespace snapper;
using namespace std;

typedef void (*cmd_fnc)( const list<string>& args );
map<string,cmd_fnc> cmds;

int print_number = 0;

Snapper* sh = NULL;

void showHelp( const list<string>& args )
    {
    cout << 
"Usage: snapper [-h] [ command [ params ] ] ...\n"
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
"\n"
"It is possible to have multiple commands on one command line.\n";
    }

void listSnap( const list<string>& args )
    {

    const Snapshots& snapshots = sh->getSnapshots();
    for (Snapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	cout << *it << endl;
	}
    }


struct CompareCallbackImpl : public CompareCallback
{
    void start() {  cout << "comparing snapshots..." << flush; }
    void stop() { cout << " done" << endl; }
};

CompareCallbackImpl compare_callback_impl;


void createSnap( const list<string>& args )
    {
    unsigned int number1 = 0;
    SnapshotType type;
    string desc;
    list<string>::const_iterator s = args.begin();
    if( s!=args.end() )
    {
	if (!toValue(*s++, type, true))
	{
	    cerr << "unknown type" << endl;
	    return;
	}
    }

    if( s!=args.end() )
	{
	if( type == POST )
	    *s >> number1;
	else
	    desc = *s;
	++s;
	}
    y2mil( "type:" << toString(type) << " desc:\"" << desc << "\" number1:" << number1 );

    switch (type)
    {
	case SINGLE:
	{
	    Snapshots::const_iterator snap1 = sh->createSingleSnapshot(desc);
	    if (print_number)
		cout << snap1->getNum() << endl;
	} break;

	case PRE:
	{
	    Snapshots::const_iterator snap1 = sh->createPreSnapshot(desc);
	    if (print_number)
		cout << snap1->getNum() << endl;
	} break;

	case POST:
	{
	    Snapshots::const_iterator snap1 = sh->getSnapshots().find(number1);
	    Snapshots::const_iterator snap2 = sh->createPostSnapshot(snap1);
	    if (print_number)
		cout << snap2->getNum() << endl;
	    sh->startBackgroundComparsion(snap1, snap2);
	} break;
    }
    }


void readNums(const list<string>& args, unsigned int& num1, unsigned int& num2)
{
    list<string>::const_iterator s = args.begin();
    if( s!=args.end() )
	{
	if( *s != "current" )
	    *s >> num1;
	++s;
	}
    if( s!=args.end() )
	{
	if( *s != "current" )
	    *s >> num2;
	++s;
	}
    y2mil("num1:" << num1 << " num2:" << num2);
}


void showDifference( const list<string>& args )
    {
    unsigned int num1 = 0;
    unsigned int num2 = 0;

    readNums(args, num1, num2);

    const Snapshots& snapshots = sh->getSnapshots();
    sh->setComparisonNums(snapshots.find(num1), snapshots.find(num2));

    const Files& files = sh->getFiles();
    for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	cout << statusToString(it->getPreToPostStatus()) << " " << it->getName() << endl;
    }


void doRollback( const list<string>& args )
    {
    unsigned int num1 = 0;
    unsigned int num2 = 0;

    readNums(args, num1, num2);

    const Snapshots& snapshots = sh->getSnapshots();
    sh->setComparisonNums(snapshots.find(num1), snapshots.find(num2));

    Files& files = sh->getFiles();
    for (Files::iterator it = files.begin(); it != files.end(); ++it)
	it->setRollback(true);

    files.doRollback();
    }


int
main(int argc, char** argv)
    {
    list<string> args;
    initDefaultLogger();
    y2mil( "argc:" << argc );

    static struct option long_options[] = {
	{ "help", 0, 0, 'h' },
	{ "print-number", 0, &print_number, 1 },
	{ 0, 0, 0, 0 }
    };
    int ch;
    while( (ch=getopt_long( argc, argv, "h", long_options, NULL )) != -1 )
	{
	switch( ch )
	    {
	    case 'h':
		showHelp(args);
		exit(0);
		break;
	    default:
		break;
	    }
	}

    cmds["list"] = listSnap;
    cmds["help"] = showHelp;
    cmds["create"] = createSnap;
    cmds["diff"] = showDifference;
    cmds["rollback"] = doRollback;

    sh = createSnapper();
    
    sh->setCompareCallback(&compare_callback_impl);

    int cnt = optind;
    while( cnt<argc )
	{
	map<string, cmd_fnc>::const_iterator c = cmds.find(argv[cnt]);
	if( c != cmds.end() )
	    {
	    list<string> args;
	    while( ++cnt<argc && cmds.find(argv[cnt])==cmds.end() )
		args.push_back(argv[cnt]);
	    (*c->second)(args);
	    }
	else 
	    {
	    y2war( "Unknown command: \"" << argv[cnt] << "\"" );
	    cerr << "Unknown command: \"" << argv[cnt] << "\"" << endl;
	    ++cnt;
	    }
	}

    deleteSnapper(sh);

    exit(EXIT_SUCCESS);
}
