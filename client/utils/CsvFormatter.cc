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

#include "client/utils/CsvFormatter.h"

using namespace std;

namespace snapper
{
    namespace cli
    {

	namespace
	{

	    const string DEFAULT_SEPARATOR = ",";


	    string double_quotes(const string value)
	    {
		return boost::algorithm::replace_all_copy(value, "\"", "\"\"" );
	    }


	    string enclose_with_quotes(const string value)
	    {
		return "\"" + value + "\"";
	    }

	}


	const string CsvFormatter::default_separator()
	{
	    return DEFAULT_SEPARATOR;
	}


	CsvFormatter::CsvFormatter(
	    vector<string> columns,
	    vector<vector<string>> rows) :
	    _columns(columns), _rows(rows), _separator(default_separator())
	{}


	CsvFormatter::CsvFormatter(
	    vector<string> columns,
	    vector<vector<string>> rows,
	    const string separator) :
	    _columns(columns), _rows(rows), _separator(separator)
	{}


	string CsvFormatter::output() const
	{
	    string cvs_output;

	    cvs_output = csv_line(_columns);

	    for (auto row : _rows)
		cvs_output += csv_line(row);

	    return cvs_output;
	}


	string CsvFormatter::csv_line(vector<string> values) const
	{
	    vector<string> csv_values;

	    for (auto value : values)
		csv_values.push_back(csv_value(value));

	    return boost::algorithm::join(csv_values, _separator) + "\n";
	}


	string CsvFormatter::csv_value(const string value) const
	{
	    string fixed_value = boost::algorithm::trim_copy(value);

	    if (has_special_chars(value))
		fixed_value = enclose_with_quotes(double_quotes(value));

	    return fixed_value;
	}


	bool CsvFormatter::has_special_chars(const string value) const
	{
	    vector<string> special_chars = { _separator, "\n", "\"" };

	    for (auto special_char : special_chars)
	    {
		if (value.find(special_char) != string::npos)
		    return true;
	    }

	    return false;
	}

    }
}