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


#ifndef SNAPPER_GET_OPTS_H
#define SNAPPER_GET_OPTS_H

#include <getopt.h>
#include <string>
#include <vector>
#include <map>

#include <snapper/Exception.h>


namespace snapper
{

    using namespace std;


    struct OptionsException : public Exception
    {
	explicit OptionsException(const string& msg) : Exception(msg) {}
    };


    struct Option
    {
	Option(const char* name, int has_arg, char c = 0)
	    : name(name), has_arg(has_arg), c(c)
	{
	}

	const char* name;
	int has_arg;
	char c;
    };


    class ParsedOpts
    {
    public:

	ParsedOpts() = default;		// TODO remove

	ParsedOpts(const map<string, string>& args) : args(args) {}

	using const_iterator = map<string, string>::const_iterator;

	bool has_option(const string& name) const { return find(name) != end(); }

	const_iterator find(const string& name) const { return args.find(name); }

	const_iterator end() const { return args.end(); }

    private:

	map<string, string> args;	// TODO make const

    };


    class GetOpts
    {
    public:

	static const vector<Option> no_options;

	GetOpts(int argc, char** argv);

	ParsedOpts parse(const vector<Option>& options);
	ParsedOpts parse(const char* command, const vector<Option>& options);

	bool has_args() const { return argc - optind > 0; }

	int num_args() const { return argc - optind; }

	const char* pop_arg() { return argv[optind++]; }

	vector<string> get_args() const { return vector<string>(&argv[optind], &argv[argc]); }

    private:

	int argc;
	char** argv;

	string make_optstring(const vector<Option>& options) const;
	vector<struct option> make_longopts(const vector<Option> options) const;

	vector<Option>::const_iterator find(const vector<Option>& options, char c) const;

    };

}

#endif
