
#include <iostream>
#include <boost/algorithm/string.hpp>

#include <snapper/AppUtil.h>

#include "GetOpts.h"
#include "text.h"

using namespace std;
using namespace snapper;


// based on getopt.cc from zypper, thanks jkupec


const struct option GetOpts::no_options[1] = {
    { 0, 0, 0, 0 }
};


void
GetOpts::init(int new_argc, char** new_argv)
{
    argc = new_argc;
    argv = new_argv;
}


GetOpts::parsed_opts
GetOpts::parse(const struct option* longopts)
{
    return parse(NULL, longopts);
}


GetOpts::parsed_opts
GetOpts::parse(const char* command, const struct option* longopts)
{
    parsed_opts result;
    opterr = 0;			// we report errors on our own

    string optstring = make_optstring(longopts);
    short2long_t short2long = make_short2long(longopts);

    while (true)
    {
	int option_index = 0;
	int c = getopt_long(argc, argv, optstring.c_str(), longopts, &option_index);

	switch (c)
	{
	    case -1:
		return result;

	    case '?':
		if (!command)
		    cerr << sformat(_("Unknown global option '%s'."), argv[optind - 1]) << endl;
		else
		    cerr << sformat(_("Unknown option '%s' for command '%s'."), argv[optind - 1], command) << endl;
		cerr << _("Try 'snapper --help' for more information.") << endl;
		exit(EXIT_FAILURE);

	    case ':':
		if (!command)
		    cerr << sformat(_("Missing argument for global option '%s'."), argv[optind - 1]) << endl;
		else
		    cerr << sformat(_("Missing argument for command option '%s'."), argv[optind - 1]) << endl;
		cerr << _("Try 'snapper --help' for more information.") << endl;
		exit(EXIT_FAILURE);

	    default:
		const char* opt = c ? short2long[c] : longopts[option_index].name;
		result[opt] = optarg ? optarg : "";
		break;
	}
    }
}


string
GetOpts::make_optstring(const struct option* longopts) const
{
    // '+' - do not permute, stop at the 1st nonoption, which is the command or an argument
    // ':' - return ':' to indicate missing arg, not '?'
    string optstring = "+:";

    for (; longopts && longopts->name; ++longopts)
    {
	if (!longopts->flag && longopts->val)
	{
	    optstring += (char) longopts->val;
	    if (longopts->has_arg == required_argument)
		optstring += ':';
	    else if (longopts->has_arg == optional_argument)
		optstring += "::";
	}
    }

    return optstring;
}


GetOpts::short2long_t
GetOpts::make_short2long(const struct option* longopts) const
{
    short2long_t result;

    for (; longopts && longopts->name; ++longopts)
    {
	if (!longopts->flag && longopts->val)
	{
	    result[longopts->val] = longopts->name;
	}
    }

    return result;
}
