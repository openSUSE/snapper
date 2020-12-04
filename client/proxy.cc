/*
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


#include <sstream>
#include <iostream>
#include <algorithm>

#include <snapper/AppUtil.h>
#include <snapper/SnapperDefines.h>

#include "utils/text.h"
#include "proxy.h"


using namespace std;


string
ProxyConfig::getSubvolume() const
{
    string subvolume;
    getValue(KEY_SUBVOLUME, subvolume);
    return subvolume;
}


bool
ProxyConfig::getValue(const string& key, string& value) const
{
    map<string, string>::const_iterator it = values.find(key);
    if (it == values.end())
	return false;

    value = it->second;
    return true;
}


bool
ProxyConfig::is_yes(const char* key) const
{
    map<string, string>::const_iterator pos = values.find(key);
    return pos != values.end() && pos->second == "yes";
}


SMD
ProxySnapshot::getSmd() const
{
    SMD smd;

    smd.description = getDescription();
    smd.cleanup = getCleanup();
    smd.userdata = getUserdata();

    return smd;
}


ProxySnapshots::iterator
ProxySnapshots::find(unsigned int num)
{
    return find_if(proxy_snapshots.begin(), proxy_snapshots.end(),
		   [num](const ProxySnapshot& x) { return x.getNum() == num; });
}


ProxySnapshots::const_iterator
ProxySnapshots::find(unsigned int num) const
{
    return find_if(proxy_snapshots.begin(), proxy_snapshots.end(),
		   [num](const ProxySnapshot& x) { return x.getNum() == num; });
}


ProxySnapshots::iterator
ProxySnapshots::findNum(const string& str)
{
    istringstream s(str);
    unsigned int num = 0;
    s >> num;

    if (s.fail() || !s.eof())
    {
	cerr << sformat(_("Invalid snapshot '%s'."), str.c_str()) << endl;
	exit(EXIT_FAILURE);
    }

    iterator ret = find(num);
    if (ret == proxy_snapshots.end())
    {
	cerr << sformat(_("Snapshot '%u' not found."), num) << endl;
	exit(EXIT_FAILURE);
    }

    return ret;
}


ProxySnapshots::const_iterator
ProxySnapshots::findNum(const string& str) const
{
    istringstream s(str);
    unsigned int num = 0;
    s >> num;

    if (s.fail() || !s.eof())
    {
	cerr << sformat(_("Invalid snapshot '%s'."), str.c_str()) << endl;
	exit(EXIT_FAILURE);
    }

    const_iterator ret = find(num);
    if (ret == proxy_snapshots.end())
    {
	cerr << sformat(_("Snapshot '%u' not found."), num) << endl;
	exit(EXIT_FAILURE);
    }

    return ret;
}


pair<ProxySnapshots::iterator, ProxySnapshots::iterator>
ProxySnapshots::findNums(const string& str, const string& delim)
{
    string::size_type pos = str.find(delim);
    if (pos == string::npos)
    {
	if (delim == "..")
	{
	    cerr << _("Missing delimiter '..' between snapshot numbers.") << endl
		 << _("See 'man snapper' for further instructions.") << endl;
	    exit(EXIT_FAILURE);
	}

	cerr << _("Invalid snapshots.") << endl;
	exit(EXIT_FAILURE);
    }

    ProxySnapshots::iterator num1 = findNum(str.substr(0, pos));
    ProxySnapshots::iterator num2 = findNum(str.substr(pos + delim.size()));

    if (num1->getNum() == num2->getNum())
    {
	cerr << _("Identical snapshots.") << endl;
	exit(EXIT_FAILURE);
    }

    return make_pair(num1, num2);
}


ProxySnapshots::const_iterator
ProxySnapshots::findPre(const_iterator post) const
{
    for (const_iterator it = begin(); it != end(); ++it)
    {
	if (it->getType() == PRE && it->getNum() == post->getPreNum())
	    return it;
    }

    return end();
}


ProxySnapshots::iterator
ProxySnapshots::findPost(iterator pre)
{
    for (iterator it = begin(); it != end(); ++it)
    {
	if (it->getType() == POST && it->getPreNum() == pre->getNum())
	    return it;
    }

    return end();
}


ProxySnapshots::const_iterator
ProxySnapshots::findPost(const_iterator pre) const
{
    for (const_iterator it = begin(); it != end(); ++it)
    {
	if (it->getType() == POST && it->getPreNum() == pre->getNum())
	    return it;
    }

    return end();
}
