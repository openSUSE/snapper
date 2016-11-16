/*
 * Copyright (c) 2016 SUSE LLC
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

#include "proxy.h"
#include "utils/text.h"


using namespace std;


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

    const_iterator ret = find_if(proxy_snapshots.begin(), proxy_snapshots.end(), [num](const ProxySnapshot& x) { return x.getNum() == num; });
    if (ret == proxy_snapshots.end())
    {
	cerr << sformat(_("Snapshot '%u' not found."), num) << endl;
	exit(EXIT_FAILURE);
    }

    return ret;
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
