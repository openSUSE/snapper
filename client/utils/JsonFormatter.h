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

#ifndef SNAPPER_JSON_FORMATTER_H
#define SNAPPER_JSON_FORMATTER_H


#include <json-c/json.h>

#include <string>
#include <ostream>


namespace snapper
{

    using namespace std;


    class JsonFormatter
    {

    public:

	JsonFormatter() : _root(json_object_new_object()) {}

	JsonFormatter(const JsonFormatter&) = delete;

	JsonFormatter& operator=(const JsonFormatter&) = delete;

	~JsonFormatter() { json_object_put(_root); }

	json_object* root() { return _root; }

	friend ostream& operator<<(ostream& stream, const JsonFormatter& json_formatter);

    private:

	json_object* _root;

    };

}

#endif
