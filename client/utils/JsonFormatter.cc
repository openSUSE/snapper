/*
 * Copyright (c) [2019] SUSE LLC
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

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "client/utils/JsonFormatter.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	namespace
	{

	    string indent(u_int indent_level)
	    {
		return string(indent_level * 2, ' ');
	    }


	    string escape(const string& value)
	    {
		string fixed_value = value;

		boost::algorithm::replace_all(fixed_value, "\\", "\\\\");
		boost::algorithm::replace_all(fixed_value, "\"", "\\\"");

		return fixed_value;
	    }


	    string quote(const string& value)
	    {
		return "\"" + value + "\"";
	    }


	    string to_json(const string& value)
	    {
		return quote(escape(boost::algorithm::trim_copy(value)));
	    }

	}


	JsonFormatter::JsonFormatter(const Data& data) :
	    _data(data), _inline(false)
	{}


	string JsonFormatter::output(u_int indent_level) const
	{
	    string first_indent = _inline ? "" : indent(indent_level);

	    return first_indent + "{" + "\n" +
		json_attributes(indent_level + 1) + "\n" +
		indent(indent_level) + "}";
	}


	string JsonFormatter::json_attributes(u_int indent_level) const
	{
	    vector<string> attributes;

	    for (auto& data_pair : _data)
	    {
		string value = data_pair.second;

		if (!skip_format_value(data_pair.first))
		    value = to_json(value);

		attributes.push_back(indent(indent_level) + to_json(data_pair.first) + ": " + value);
	    }

	    return boost::algorithm::join(attributes, ",\n");
	}


	bool JsonFormatter::skip_format_value(const string& key) const
	{
	    auto it = find(_skip_format_values.begin(), _skip_format_values.end(), key);

	    if (it != _skip_format_values.end())
		return true;

	    return false;
	}


	JsonFormatter::JsonFormatter::List::List(const vector<string>& data) :
	    _data(data)
	{}


	string JsonFormatter::List::output(u_int indent_level) const
	{
	    return "[\n" +
		boost::algorithm::join(_data, ",\n") + "\n" +
		indent(indent_level) + "]";
	}

    }
}