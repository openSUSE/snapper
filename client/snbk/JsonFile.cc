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


#include <stdio.h>
#include <sys/stat.h>
#include <functional>
#include <memory>

#include "snapper/Exception.h"
#include "snapper/AppUtil.h"

#include "JsonFile.h"


namespace snapper
{

    class JsonTokener
    {
    public:

	JsonTokener()
	    : p(json_tokener_new())
	{
	    if (!p)
		SN_THROW(Exception("out of memory"));
	}

	~JsonTokener()
	{
	    json_tokener_free(p);
	}

	json_tokener* get() { return p; }

    private:

	json_tokener* p;

    };


    JsonFile::JsonFile(const vector<string>& lines)
    {
	JsonTokener tokener;

	for (const string& line : lines)
	{
	    root = json_tokener_parse_ex(tokener.get(), line.c_str(), line.size());

	    switch (json_tokener_get_error(tokener.get()))
	    {
		case json_tokener_continue:
		    continue;

		case json_tokener_success:
		    return;

		default:
		    break;
	    }
	}

	SN_THROW(Exception("json parser failed"));
    }


    JsonFile::JsonFile(const string& filename)
    {
	FILE* fp = fopen(filename.c_str(), "r");
	if (!fp)
	    SN_THROW(Exception(sformat("open for json file '%s' failed", filename.c_str())));

	struct stat st;
	if (fstat(fileno(fp), &st) != 0)
	{
	    fclose(fp);
	    SN_THROW(Exception(sformat("stat for json file '%s' failed", filename.c_str())));
	}

	vector<char> data(st.st_size);
	if (fread(data.data(), 1, st.st_size, fp) != (size_t)(st.st_size))
	{
	    fclose(fp);
	    SN_THROW(Exception(sformat("read for json file '%s' failed", filename.c_str())));
	}

	if (fclose(fp) != 0)
	    SN_THROW(Exception(sformat("close for json file '%s' failed", filename.c_str())));

	JsonTokener tokener;

	root = json_tokener_parse_ex(tokener.get(), data.data(), st.st_size);

	if (json_tokener_get_error(tokener.get()) != json_tokener_success)
	{
	    json_object_put(root);
	    SN_THROW(Exception(sformat("parsing json file '%s' failed", filename.c_str())));
	}

	if (tokener.get()->char_offset != st.st_size)
	{
	    json_object_put(root);
	    SN_THROW(Exception(sformat("excessive content in json file '%s'", filename.c_str())));
	}
    }


    JsonFile::~JsonFile()
    {
	json_object_put(root);
    }


    template<>
    bool
    get_child_value(json_object* parent, const char* name, string& value)
    {
	json_object* child;

	if (!json_object_object_get_ex(parent, name, &child))
	    return false;

	if (!json_object_is_type(child, json_type_string))
	    return false;

	value = json_object_get_string(child);

	return true;
    }


    template<>
    bool
    get_child_value(json_object* parent, const char* name, bool& value)
    {
	json_object* child;

	if (!json_object_object_get_ex(parent, name, &child))
	    return false;

	if (!json_object_is_type(child, json_type_boolean))
	    return false;

	value = json_object_get_boolean(child);

	return true;
    }


    template<>
    bool
    get_child_value(json_object* parent, const char* name, unsigned int& value)
    {
	static_assert(sizeof(unsigned int) <= 4, "unsigned int wider than 32 bit");

	json_object* child;

	if (!json_object_object_get_ex(parent, name, &child))
	    return false;

	if (!json_object_is_type(child, json_type_int) && !json_object_is_type(child, json_type_string))
	    return false;

	value = json_object_get_int(child);

	return true;
    }


    bool
    get_child_nodes(json_object* parent, const char* name, vector<json_object*>& children)
    {
	json_object* child;

	if (!json_object_object_get_ex(parent, name, &child))
	    return false;

	if (!json_object_is_type(child, json_type_array))
	    return false;

	children.clear();

	size_t s = json_object_array_length(child);
	for (size_t i = 0; i < s; ++i)
	    children.push_back(json_object_array_get_idx(child, i));

	return true;
    }

}
