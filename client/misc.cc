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

#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>

#include <snapper/AppUtil.h>
#include <snapper/SystemCmd.h>

#include "utils/text.h"

#include "misc.h"


Snapshots::iterator
read_num(Snapper* snapper, const string& str)
{
    Snapshots& snapshots = snapper->getSnapshots();

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


unsigned int
read_num(const string& str)
{
    istringstream s(str);
    unsigned int num = 0;
    s >> num;

    if (s.fail() || !s.eof())
    {
	cerr << sformat(_("Invalid snapshot '%s'."), str.c_str()) << endl;
	exit(EXIT_FAILURE);
    }

    return num;
}


pair<unsigned int, unsigned int>
read_nums(const string& str, const string& delim)
{
    string::size_type pos = str.find(delim);
    if (pos == string::npos)
    {
	cerr << _("Invalid snapshots.") << endl;
	exit(EXIT_FAILURE);
    }

    unsigned int num1 = read_num(str.substr(0, pos));
    unsigned int num2 = read_num(str.substr(pos + delim.size()));

    if (num1 == num2)
    {
	cerr << _("Identical snapshots.") << endl;
	exit(EXIT_FAILURE);
    }

    return pair<unsigned int, unsigned int>(num1, num2);
}


map<string, string>
read_userdata(const string& s, const map<string, string>& old)
{
    if (s.empty())
    {
	cerr << _("Empty userdata.") << endl;
	exit(EXIT_FAILURE);
    }

    list<string> tmp;
    boost::split(tmp, s, boost::is_any_of(","), boost::token_compress_on);
    if (tmp.empty())
    {
	cerr << _("Empty userdata.") << endl;
	exit(EXIT_FAILURE);
    }

    map<string, string> userdata = old;

    for (list<string>::const_iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
	string::size_type pos = it->find("=");
	if (pos == string::npos)
	{
	    cerr << sformat(_("Userdata '%s' does not include '=' sign."), it->c_str()) << endl;
	    exit(EXIT_FAILURE);
	}

	string key = boost::trim_copy(it->substr(0, pos));
	string value = boost::trim_copy(it->substr(pos + 1));

	if (key.empty())
	{
	    cerr << sformat(_("Userdata '%s' has empty key."), it->c_str()) << endl;
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


map<string, string>
read_configdata(const list<string>& l, const map<string, string>& old)
{
    if (l.empty())
    {
	cerr << _("Empty configdata.") << endl;
	exit(EXIT_FAILURE);
    }

    map<string, string> configdata = old;

    for (list<string>::const_iterator it = l.begin(); it != l.end(); ++it)
    {
	string::size_type pos = it->find("=");
	if (pos == string::npos)
	{
	    cerr << sformat(_("Configdata '%s' does not include '=' sign."), it->c_str()) << endl;
	    exit(EXIT_FAILURE);
	}

	string key = boost::trim_copy(it->substr(0, pos));
	string value = boost::trim_copy(it->substr(pos + 1));

	if (key.empty())
	{
	    cerr << sformat(_("Configdata '%s' has empty key."), it->c_str()) << endl;
	    exit(EXIT_FAILURE);
	}

	configdata[key] = value;
    }

    return configdata;
}


string
username(uid_t uid)
{
    string username;
    gid_t gid;

    if (!get_uid_username_gid(uid, username, gid))
	return sformat("unknown (%d)", uid);

    return username;
}


Differ::Differ()
    : command(DIFFBIN " --new-file --unified"), extensions()
{
}


void
Differ::run(const string& f1, const string& f2) const
{
    string tmp = command;
    if (!extensions.empty())
	tmp += " " + extensions;
    tmp += " " + quote(f1) + " " + quote(f2);

    SystemCmd cmd(tmp);

    for (const string& line : cmd.stdout())
	cout << line << endl;

    for (const string& line : cmd.stderr())
	cerr << line << endl;
}
