/*
 * Copyright (c) [2004-2015] Novell, Inc.
 * Copyright (c) [2020-2022] SUSE LLC
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
#include <memory>

#include "snapper/Exception.h"


namespace snapper
{
    using std::string;
    using std::vector;
    using std::map;


    enum class Compression { NONE, GZIP, ZSTD };


    bool
    is_available(Compression compression);


    string
    add_extension(Compression compression, const string& name);


    class AsciiFileReader
    {
    public:

	AsciiFileReader(int fd, Compression compression);
	AsciiFileReader(FILE* fin, Compression compression);
	AsciiFileReader(const string& name, Compression compression);

	/**
	 * Exceptions from close are ignored. Use close() explicitly.
	 */
	~AsciiFileReader();

	bool read_line(string& line);

	void close();

    private:

	class Impl;

	std::unique_ptr<Impl> impl;

    };


    class AsciiFileWriter
    {
    public:

	AsciiFileWriter(int fd, Compression compression);
	AsciiFileWriter(FILE* fout, Compression compression);
	AsciiFileWriter(const string& name, Compression compression);

	/**
	 * Exceptions from close are ignored. Use close() explicitly.
	 */
	~AsciiFileWriter();

	void write_line(const string& line);

	void close();

    public:

	class Impl;

	std::unique_ptr<Impl> impl;

    };


    class AsciiFile
    {
    public:

	explicit AsciiFile(const string& name, bool remove_empty = false);

	/**
	 * Does not call save().
	 */
	~AsciiFile();

	string get_name() const { return name; }
	void set_name(const string& name) { AsciiFile::name = name; }

	void reload();

	void save();

	void log_content() const;

	bool empty() const { return lines.empty(); }

	void clear() { lines.clear(); }

	void push_back(const string& line) { lines.push_back(line); }

	vector<string>& get_lines() { return lines; }
	const vector<string>& get_lines() const { return lines; }

    protected:

	vector<string> lines;

    private:

	string name;
	bool remove_empty = false;

    };


    class SysconfigFile : protected AsciiFile
    {
    public:

	struct InvalidKeyException : public Exception
	{
	    explicit InvalidKeyException() : Exception("invalid key") {}
	};

	SysconfigFile(const string& name);

	/**
	 * Calls save(). Exceptions from save are ignored. Use save() explicitly in
	 * needed.
	 */
	virtual ~SysconfigFile();

	void set_name(const string& name) { AsciiFile::set_name(name); }

	/**
	 * Save if modified.
	 */
	void save();

	virtual void check_key(const string& key) const;

	virtual void set_value(const string& key, bool value);
	bool get_value(const string& key, bool& value) const;

	virtual void set_value(const string& key, const char* value);
	virtual void set_value(const string& key, const string& value);
	bool get_value(const string& key, string& value) const;

	virtual void set_value(const string& key, const vector<string>& values);
	bool get_value(const string& key, vector<string>& values) const;

	map<string, string> get_all_values() const;

    private:

	bool modified = false;

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
