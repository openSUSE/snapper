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

#ifndef SNAPPER_CLI_CSV_FORMATTER_H
#define SNAPPER_CLI_CSV_FORMATTER_H

#include <string>
#include <vector>


namespace snapper
{

    using namespace std;


    namespace cli
    {

	class CsvFormatter
	{

	public:

	    static const string default_separator;

	    CsvFormatter(const vector<string>& header, const vector<vector<string>>& rows,
			 const string& separator)
		: header(header), rows(rows), separator(separator)
	    {
	    }

	    string str() const;

	private:

	    string csv_line(const vector<string>& values) const;

	    string csv_value(const string& value) const;

	    bool has_special_chars(const string& value) const;

	    string double_quotes(const string& value) const;

	    string enclose_with_quotes(const string& value) const;

	    const vector<string> header;
	    const vector<vector<string>> rows;
	    const string separator;

	};

    }

}

#endif
