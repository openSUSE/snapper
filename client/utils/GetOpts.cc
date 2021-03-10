/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2020] SUSE LLC
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


#include <algorithm>

#include <snapper/AppUtil.h>

#include "GetOpts.h"
#include "text.h"


namespace snapper
{

    using namespace std;


    const struct vector<Option> GetOpts::no_options = {};


    GetOpts::GetOpts(int argc, char** argv)
	: argc(argc), argv(argv)
    {
	opterr = 0;		// we report errors on our own
    }


    ParsedOpts
    GetOpts::parse(const vector<Option>& options)
    {
	return parse(nullptr, options);
    }


    ParsedOpts
    GetOpts::parse(const char* command, const vector<Option>& options)
    {
	string optstring = make_optstring(options);
	vector<struct option> longopts = make_longopts(options);

	map<string, string> result;

	while (true)
	{
	    int option_index = 0;
	    int c = getopt_long(argc, argv, optstring.c_str(), &longopts[0], &option_index);

	    switch (c)
	    {
		case -1:
		{
		    return result;
		}

		case '?':
		{
		    string opt;
		    if (optopt != 0)
			opt = string("-") + (char)(optopt);
		    else
			opt = argv[optind - 1];

		    string msg;
		    if (!command)
			msg = sformat(_("Unknown global option '%s'."), opt.c_str());
		    else
			msg = sformat(_("Unknown option '%s' for command '%s'."), opt.c_str(), command);

		    SN_THROW(OptionsException(msg));
		    break;
		}

		case ':':
		{
		    string opt;
		    if (optopt != 0)
		    {
			vector<Option>::const_iterator it = find(options, optopt);
			if (it == options.end())
			    SN_THROW(OptionsException("option not found"));

			opt = string("--") + it->name;
		    }
		    else
		    {
			opt = argv[optind - 1];
		    }

		    string msg;
		    if (!command)
			msg = sformat(_("Missing argument for global option '%s'."), opt.c_str());
		    else
			msg = sformat(_("Missing argument for command option '%s'."), opt.c_str());

		    SN_THROW(OptionsException(msg));
		    break;
		}

		default:
		{
		    vector<Option>::const_iterator it = c ? find(options, c) : options.begin() + option_index;
		    if (it == options.end())
			SN_THROW(OptionsException("option not found"));

		    result[it->name] = optarg ? optarg : "";
		    break;
		}
	    }
	}
    }


    vector<Option>::const_iterator
    GetOpts::find(const vector<Option>& options, char c) const
    {
	return find_if(options.begin(), options.end(), [c](const Option& option)
	    { return option.c == c; }
	);
    }


    string
    GetOpts::make_optstring(const vector<Option>& options) const
    {
	// '+' - do not permute, stop at the 1st non-option, which is the command or an argument
	// ':' - return ':' to indicate missing arg, not '?'
	string optstring = "+:";

	for (const Option& option : options)
	{
	    if (option.c == 0)
		continue;

	    optstring += option.c;

	    switch (option.has_arg)
	    {
		case no_argument:
		    break;

		case required_argument:
		    optstring += ':';
		    break;
	    }
	}

	return optstring;
    }


    vector<struct option>
    GetOpts::make_longopts(const vector<Option>& options) const
    {
	vector<struct option> ret;

	for (const Option& option : options)
	    ret.push_back({ option.name, option.has_arg, nullptr, option.c });

	ret.push_back({ nullptr, 0, nullptr, 0 });

	return ret;
    }

}
