/*
 * Copyright (c) 2021 SUSE LLC
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


#include <locale>
#include <boost/algorithm/string.hpp>

#include <snapper/Exception.h>
#include <snapper/SnapperTmpl.h>

#include "Limit.h"
#include "HumanString.h"


namespace snapper
{
    using namespace std;


    void
    Limit::parse(const string& s)
    {
	// try plain float

	double v;
	istringstream a(boost::trim_copy(s, locale::classic()));
	classic(a);
	a >> v;

	if (!a.fail() && a.eof())
	{
	    fraction = v;
	    type = FRACTION;
	    return;
	}

	// try size

	try
	{
	    absolute = humanstring_to_byte(s, true);
	    type = ABSOLUTE;
	    return;
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);
	}

	SN_THROW(Exception("failed to parse limit"));
    }


    ostream&
    operator<<(ostream& str, const Limit& smart_size)
    {
	switch (smart_size.type)
	{
	    case Limit::FRACTION:
		str << smart_size.fraction;
		break;

	    case Limit::ABSOLUTE:
		str << byte_to_humanstring(smart_size.absolute, false, 2);
		break;
	}

	return str;
    }


    bool
    MaxUsedLimit::is_enabled() const
    {
	switch (type)
	{
	    case FRACTION:
		return fraction < 1.0;

	    case ABSOLUTE:
		return true;
	}

	return false;
    }


    bool
    MaxUsedLimit::is_satisfied(unsigned long long size, unsigned long long used) const
    {
	switch (type)
	{
	    case FRACTION:
		return (double)(used) / (double)(size) < fraction;

	    case ABSOLUTE:
		return used < absolute;
	}

	return false;
    }


    bool
    MinFreeLimit::is_enabled() const
    {
	switch (type)
	{
	    case FRACTION:
		return fraction > 0.0;

	    case ABSOLUTE:
		return absolute > 0;
	}

	return false;
    }


    bool
    MinFreeLimit::is_satisfied(unsigned long long size, unsigned long long free) const
    {
	switch (type)
	{
	    case FRACTION:
		return (double)(free) / (double)(size) > fraction;

	    case ABSOLUTE:
		return free > absolute;
	}

	return false;
    }

}
