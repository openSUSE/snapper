/*
 * Copyright (c) [2019-2020] SUSE LLC
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

#ifndef SNAPPER_CSV_FORMATTER_H
#define SNAPPER_CSV_FORMATTER_H


#include <string>
#include <vector>
#include <ostream>


namespace snapper
{

    using namespace std;


    class CsvFormatter
    {

    public:

	static const string default_separator;

	CsvFormatter(const string& separator) : separator(separator) {}

	CsvFormatter(const CsvFormatter&) = delete;

	CsvFormatter& operator=(const CsvFormatter&) = delete;

	vector<string>& header() { return _header; }
	vector<vector<string>>&  rows() { return _rows; }

	friend ostream& operator<<(ostream& stream, const CsvFormatter& csv_formatter);

    private:

	string csv_line(const vector<string>& values) const;

	string csv_value(const string& value) const;

	bool has_special_chars(const string& value) const;

	string double_quotes(const string& value) const;

	string enclose_with_quotes(const string& value) const;

	const string separator;

	vector<string> _header;
	vector<vector<string>> _rows;

    };

}

#endif
