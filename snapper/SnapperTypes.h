/*
 * Copyright (c) [2004-2011] Novell, Inc.
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


#ifndef SNAPPER_TYPES_H
#define SNAPPER_TYPES_H


#include <string>
#include <vector>
#include <ostream>
#include <boost/algorithm/string.hpp>

#include "snapper/Regex.h"


namespace snapper
{
    using std::string;
    using std::vector;


    struct regex_matches
    {
	regex_matches(const Regex& t) : val(t) {}
	bool operator()(const string& s) const { return val.match(s); }
	const Regex& val;
    };

    struct string_starts_with
    {
	string_starts_with(const string& t) : val(t) {}
	bool operator()(const string& s) const { return boost::starts_with(s, val); }
	const string& val;
    };

    struct string_contains
    {
	string_contains(const string& t) : val(t) {}
	bool operator()(const string& s) const { return boost::contains(s, val); }
	const string& val;
    };


    template <class Pred>
    vector<string>::iterator
    find_if(vector<string>& lines, Pred pred)
    {
	return std::find_if(lines.begin(), lines.end(), pred);
    }

    template <class Pred>
    vector<string>::const_iterator
    find_if(const vector<string>& lines, Pred pred)
    {
	return std::find_if(lines.begin(), lines.end(), pred);
    }

}


#endif
