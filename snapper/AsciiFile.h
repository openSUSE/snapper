/*
 * Copyright (c) [2004-2012] Novell, Inc.
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


namespace snapper
{
    using std::string;
    using std::vector;


    class AsciiFileReader
    {
    public:

	AsciiFileReader(int fd);
	AsciiFileReader(FILE* file);
	AsciiFileReader(const string& filename);
	~AsciiFileReader();

	bool getline(string& line);

    private:

	FILE* file;
	char* buffer;
	size_t len;

    };


    class AsciiFile
    {
    public:

	explicit AsciiFile(const char* name, bool remove_empty = false);
	explicit AsciiFile(const string& name, bool remove_empty = false);

	string name() const { return Name_C; }

	void reload();
	bool save();

	void logContent() const;

	void clear() { Lines_C.clear(); }
	void push_back(const string& line) { Lines_C.push_back(line); }

	vector<string>& lines() { return Lines_C; }
	const vector<string>& lines() const { return Lines_C; }

    protected:

	const string Name_C;
	const bool remove_empty;

	vector<string> Lines_C;

    };


    class SysconfigFile : protected AsciiFile
    {
    public:

	SysconfigFile(const char* name) : AsciiFile(name), modified(false) {}
	SysconfigFile(const string& name) : AsciiFile(name), modified(false) {}
	~SysconfigFile() { if (modified) save(); }

	void setValue(const string& key, bool value);
	bool getValue(const string& key, bool& value) const;

	void setValue(const string& key, const string& value);
	bool getValue(const string& key, string& value) const;

	void setValue(const string& key, const vector<string>& values);
	bool getValue(const string& key, vector<string>& values) const;

	map<string, string> getAllValues() const;

    private:

	bool modified;

    };

}


#endif
