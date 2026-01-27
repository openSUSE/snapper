/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2024] SUSE LLC
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


#include "OutputOptions.h"

#include <snapper/Exception.h>

#include "text.h"


namespace snapper
{

    using namespace std;


    string
    any_to_string(const OutputOptions& output_options, const std::any& value)
    {
	if (value.type() == typeid(nullptr_t))
	{
	    return "";
	}

	if (value.type() == typeid(bool))
	{
	    if (output_options.human)
		return std::any_cast<bool>(value) ? _("yes") : _("no");
	    else
		return std::any_cast<bool>(value) ? "yes" : "no";
	}

	if (value.type() == typeid(unsigned int))
	{
	    return to_string(std::any_cast<unsigned int>(value));
	}

	if (value.type() == typeid(string))
	{
	    return std::any_cast<string>(value).c_str();
	}

	SN_THROW(Exception("invalid type in any_to_string"));
	__builtin_unreachable();
    }


    json_object*
    any_to_json(const OutputOptions& output_options, const std::any& value)
    {
	if (value.type() == typeid(nullptr_t))
	{
	    return nullptr;
	}

	if (value.type() == typeid(bool))
	{
	    return json_object_new_boolean(std::any_cast<bool>(value));
	}

	if (value.type() == typeid(unsigned int))
	{
	    return json_object_new_int(std::any_cast<unsigned int>(value));
	}

	if (value.type() == typeid(string))
	{
	    return json_object_new_string(std::any_cast<string>(value).c_str());
	}

	SN_THROW(Exception("invalid type in any_to_json"));
	__builtin_unreachable();
    }

}
