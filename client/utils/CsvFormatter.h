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

#ifndef SNAPPER_CLI_CSV_FORMATTER_H
#define SNAPPER_CLI_CSV_FORMATTER_H

#include <string>
#include <vector>

namespace snapper
{
    namespace cli
    {

	class CsvFormatter
	{

	public:

	    static const std::string default_separator();

	    CsvFormatter(
		std::vector<std::string> columns,
		std::vector<std::vector<std::string>> rows);

	    CsvFormatter(
		std::vector<std::string> columns,
		std::vector<std::vector<std::string>> rows,
		const std::string separator);

	    std::string output() const;

	private:

	    std::string csv_line(std::vector<std::string> values) const;

	    std::string csv_value(const std::string value) const;

	    bool has_special_chars(const std::string value) const;

	    std::vector<std::string> _columns;

	    std::vector<std::vector<std::string>> _rows;

	    const std::string _separator;

	};

    }
}

#endif
