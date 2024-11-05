/*
 * Copyright (c) [2017-2024] SUSE LLC
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


#ifndef SNAPPER_JSON_FILE_H
#define SNAPPER_JSON_FILE_H


#include <json-c/json.h>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>


namespace snapper
{

    using std::string;
    using std::vector;


    /*
     * The user might expect more use of const here but in the end it does not work out
     * since json_object_get_string does not take a const (likely due to reference
     * counting).
     */


    class JsonFile : private boost::noncopyable
    {

    public:

	JsonFile(const vector<string>& lines);

	JsonFile(const string& filename);

	~JsonFile();

	json_object* get_root() { return root; }

    private:

	json_object* root = nullptr;

    };


    template<typename Type> bool
    get_child_value(json_object* parent, const char* name, Type& value);


    bool
    get_child_nodes(json_object* parent, const char* name, vector<json_object*>& children);

}


#endif
