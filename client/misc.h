/*
 * Copyright (c) [2011-2014] Novell, Inc.
 * Copyright (c) [2020-2023] SUSE LLC
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
#include <boost/algorithm/string.hpp>

#include <snapper/Snapper.h>
#include <snapper/AppUtil.h>
#include <snapper/Enum.h>

#include "client/utils/text.h"
#include "client/utils/GetOpts.h"
#include "proxy/proxy.h"


namespace snapper
{

unsigned int
read_num(const string& str);

map<string, string>
read_userdata(const string& s, const map<string, string>& old = map<string, string>());

string
show_userdata(const map<string, string>& userdata);

map<string, string>
read_configdata(const vector<string>& v, const map<string, string>& old = map<string, string>());

string
username(uid_t uid);

    unique_ptr<const Filesystem>
    get_filesystem(const ProxyConfig& config, const string& target_root);


struct Differ
{
    Differ();

    void run(const string& f1, const string& f2) const;

    string command;
    string extensions;
};


    /**
     * Return a string listing the possible enum values. E.g. "Use auto, classic or
     * transactional." for possible_enum_values<Ambit>().
     */
    template<class Column>
    string
    possible_enum_values()
    {
	const vector<string>& names = EnumInfo<Column>::names;

	string ret;

	for (vector<string>::const_iterator it = names.begin(); it != names.end(); ++it)
	{
	    if (it == names.begin())
	    {
		ret = *it;
	    }
	    else if (it == names.end() - 1)
	    {
		// TRANSLATORS: used to construct list of values
		// %1$s is replaced by first value
		// %2$s is replaced by second value
		ret = sformat(_("%1$s or %2$s"), ret.c_str(), it->c_str());
	    }
	    else
	    {
		// TRANSLATORS: used to construct list of values
		// %1$s is replaced by first value
		// %2$s is replaced by second value
		ret = sformat(_("%1$s, %2$s"), ret.c_str(), it->c_str());
	    }
	}

	// TRANSLATORS: a list of possible values
	// %1$s is replaced by list of possible values
	return sformat(_("Use %1$s."), ret.c_str());
    }


    /**
     * Transform a string with comma separated columns to a vector of columns.
     */
    template<class Column>
    vector<Column>
    parse_columns(const string& columns)
    {
	vector<string> tmp;
	boost::split(tmp, columns, boost::is_any_of(","), boost::token_compress_on);

	vector<Column> ret;

	for (const string& str : tmp)
	{
	    Column column;
	    if (!toValue(str, column, false))
	    {
		string error = sformat(_("Invalid column '%s'."), str.c_str()) + '\n' +
		    possible_enum_values<Column>();
		SN_THROW(OptionsException(error));
	    }

	    ret.push_back(column);
	}

	return ret;
    }

}
