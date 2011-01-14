
#include <stdlib.h>
#include <getopt.h>
#include <iostream>

#include <snapper/Snapper.h>
#include <snapper/AppUtil.h>
#include <snapper/SnapperTmpl.h>
#include <snapper/Files.h>

using namespace snapper;
using namespace std;

typedef void (*cmd_fnc)( const list<string>& args );
map<string,cmd_fnc> cmds;

void showHelp( const list<string>& args )
    {
    cout << 
"Usage: snapper_cl [-h] [ command [ params ] ] ...\n"
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
    listSnapshots();
    }

void createSnap( const list<string>& args )
    {
    unsigned number = 0;
    string type;
    string desc;
    list<string>::const_iterator s = args.begin();
    if( s!=args.end() )
	type = *s++;
    if( s!=args.end() )
	{
	if( type == "post" )
	    *s >> number;
	else
	    desc = *s;
	++s;
	}
    y2mil( "type:" << type << " desc:\"" << desc << "\" number:" << number );
    if( type=="single" )
	createSingleSnapshot(desc);
    else if( type=="pre" )
	createPreSnapshot(desc);
    else if( type=="post" )
    {
	unsigned int num2 = createPostSnapshot(number);
	startBackgroundComparsion(number, num2);
    }
    else
	y2war( "unknown type:\"" << type << "\"" );
    }

void showDifference( const list<string>& args )
    {
    unsigned n1 = 0;
    unsigned n2 = 0;
    list<string>::const_iterator s = args.begin();
    if( s!=args.end() )
	{
	if( *s != "CURRENT" )
	    *s >> n1;
	++s;
	}
    if( s!=args.end() )
	{
	if( *s != "CURRENT" )
	    *s >> n2;
	++s;
	}
    y2mil( "n1:" << n1 << " n2:" << n2 );

    setComparisonNums(n1, n2);

    for (vector<File>::const_iterator it = filelist.begin(); it != filelist.end(); ++it)
	cout << statusToString(it->pre_to_post_status) << " " << it->name << endl;
    }

int
main(int argc, char** argv)
    {
    list<string> args;
    initDefaultLogger();
    y2mil( "argc:" << argc );

    static struct option long_options[] = {
	{ "help", 0, 0, 'h' },
	{ 0, 0, 0, 0 } };
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

    for( int i=0; i<argc; i++ )
	args.push_back(argv[i]);
    y2mil( "argv:" << args );

    map<string,cmd_fnc>::iterator c;
    int cnt=1;
    while( cnt<argc )
	{
	if( (c=cmds.find(argv[cnt]))!=cmds.end() )
	    {
	    args.clear();
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
    exit(EXIT_SUCCESS);
}
