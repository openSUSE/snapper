/*
 * Copyright (c) [2004-2015] Novell, Inc.
 * Copyright (c) 2020 SUSE LLC
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


#ifndef SNAPPER_ASCII_FILE_H
#define SNAPPER_ASCII_FILE_H


#include <string>
#include <vector>
#include <map>

#include "snapper/Exception.h"


namespace snapper
{
    using std::string;
    using std::vector;
    using std::map;


    class AsciiFileReader
    {
    public:

	AsciiFileReader(int fd);
	AsciiFileReader(FILE* file);
	AsciiFileReader(const string& filename);
	~AsciiFileReader();

	bool getline(string& line);

    private:

	FILE* file = nullptr;
	char* buffer = nullptr;
	size_t len = 0;

    };


    class AsciiFile
    {
    public:

	explicit AsciiFile(const char* name, bool remove_empty = false);
	explicit AsciiFile(const string& name, bool remove_empty = false);

	string name() const { return Name_C; }

	void setName(const string& name) { AsciiFile::Name_C = name; }

	void reload();
	bool save();

	void logContent() const;

	void clear() { Lines_C.clear(); }
	void push_back(const string& line) { Lines_C.push_back(line); }

	vector<string>& lines() { return Lines_C; }
	const vector<string>& lines() const { return Lines_C; }

    protected:

	vector<string> Lines_C;

    private:

	string Name_C;
	bool remove_empty;

    };


    class SysconfigFile : protected AsciiFile
    {
    public:

	struct InvalidKeyException : public Exception
	{
	    explicit InvalidKeyException() : Exception("invalid key") {}
	};

	SysconfigFile(const char* name) : AsciiFile(name), modified(false) {}
	SysconfigFile(const string& name) : AsciiFile(name), modified(false) {}
	virtual ~SysconfigFile() { if (modified) save(); }

	void setName(const string& name) { AsciiFile::setName(name); }

	void save();

	virtual void checkKey(const string& key) const;

	virtual void setValue(const string& key, bool value);
	bool getValue(const string& key, bool& value) const;

	virtual void setValue(const string& key, const char* value);
	virtual void setValue(const string& key, const string& value);
	bool getValue(const string& key, string& value) const;

	virtual void setValue(const string& key, const vector<string>& values);
	bool getValue(const string& key, vector<string>& values) const;

	map<string, string> getAllValues() const;

    private:

	bool modified;

	struct ParsedLine
	{
	    string key;
	    string value;
	    string comment;
	};

	bool parse_line(const string& line, ParsedLine& parsed_line) const;

    };

}


#endif
