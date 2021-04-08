/*
 * Copyright (c) [2016-2021] SUSE LLC
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

#include <snapper/Exception.h>
#include <snapper/SnapperTmpl.h>

#include "Range.h"


namespace snapper
{

    using namespace std;


    void
    Range::parse(const string& s)
    {
	string::size_type pos = s.find('-');
	if (pos == string::npos)
	{
	    size_t v;

	    istringstream a(s);
	    classic(a);
	    a >> v;
	    if (a.fail() || !a.eof())
		SN_THROW(Exception("failed to parse range"));

	    min = max = v;
	}
	else
	{
	    size_t v1, v2;

	    istringstream a(s.substr(0, pos));
	    classic(a);
	    a >> v1;
	    if (a.fail() || !a.eof())
		SN_THROW(Exception("failed to parse range"));

	    istringstream b(s.substr(pos + 1));
	    classic(b);
	    b >> v2;
	    if (b.fail() || !b.eof())
		SN_THROW(Exception("failed to parse range"));

	    if (v1 > v2)
		SN_THROW(Exception("failed to parse range"));

	    min = v1;
	    max = v2;
	}
    }


    ostream&
    operator<<(ostream& s, const Range& range)
    {
	return s << range.min << "-" << range.max;
    }

}
