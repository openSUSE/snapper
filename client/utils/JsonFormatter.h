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

#ifndef SNAPPER_CLI_JSON_FORMATTER_H
#define SNAPPER_CLI_JSON_FORMATTER_H

#include <string>
#include <vector>
#include <utility>

namespace snapper
{
    namespace cli
    {

	/* Very simplistic JSON formatter, but enough for current requirements.
	 *
	 * If needed, this could be replaced by some modern library, for example:
	 * https://github.com/nlohmann/json
	 */
	class JsonFormatter
	{

	public:

	    class List;

	    using Data = std::vector<std::pair<std::string, std::string>>;

	    JsonFormatter(const Data& data);

	    void skip_format_values(const std::vector<std::string>& skip_format_values)
	    {
		_skip_format_values = skip_format_values;
	    }

	    void set_inline(bool is_inline)
	    {
		_inline = is_inline;
	    }

	    std::string output(u_int indent_level = 0) const;

	private:

	    std::string json_attributes(u_int indent_level) const;

	    bool skip_format_value(const std::string& key) const;

	    const Data& _data;

	    std::vector<std::string> _skip_format_values;

	    bool _inline;
	};


	class JsonFormatter::List
	{

	public:

	    List(const std::vector<std::string>& data);

	    std::string output(u_int indent_level = 0) const;

	private:

	    const std::vector<std::string>& _data;

	};

    }
}

#endif
