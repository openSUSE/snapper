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


#include <string>
#include <sstream>

#include "Range.h"

using namespace std;


istream&
operator>>(istream& s, Range& range)
{
    string value;
    s >> value;

    string::size_type pos = value.find('-');
    if (pos == string::npos)
    {
	size_t v;

	istringstream a(value);
	a >> v;
	if (a.fail() || !a.eof())
	{
	    s.setstate(ios::failbit);
	    return s;
	}

	range.min = range.max = v;
    }
    else
    {
	size_t v1, v2;

	istringstream a(value.substr(0, pos));
	a >> v1;
	if (a.fail() || !a.eof())
	{
	    s.setstate(ios::failbit);
	    return s;
	}

	istringstream b(value.substr(pos + 1));
	b >> v2;
	if (b.fail() || !b.eof())
	{
	    s.setstate(ios::failbit);
	    return s;
	}

	if (v1 > v2)
	{
	    s.setstate(ios::failbit);
	    return s;
	}

	range.min = v1;
	range.max = v2;
    }

    return s;
}


ostream&
operator<<(ostream& s, const Range& range)
{
    return s << range.min << "-" << range.max;
}
