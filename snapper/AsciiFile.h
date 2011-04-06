/*
 * Copyright (c) [2004-2011] Novell, Inc.
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


#ifndef ASCII_FILE_H
#define ASCII_FILE_H


#include <string>
#include <vector>
#include <algorithm>


namespace snapper
{
    using std::string;
    using std::vector;


    class AsciiFileReader
    {
    public:

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

	bool reload();
	bool save();

	void logContent() const;

	void append( const string& Line_Cv );
	void append( const vector<string>& Lines_Cv );
	void insert( unsigned int Before_iv, const string& Line_Cv );
	void clear();
	void remove( unsigned int Start_iv, unsigned int Cnt_iv );
	void replace( unsigned int Start_iv, unsigned int Cnt_iv,
		      const string& Line_Cv );
	void replace( unsigned int Start_iv, unsigned int Cnt_iv,
		      const vector<string>& Line_Cv );

	const string& operator []( unsigned int Index_iv ) const;
	string& operator []( unsigned int Index_iv );

	template <class Pred>
	int find_if_idx(Pred pred) const
	{
	    vector<string>::const_iterator it = std::find_if(Lines_C.begin(), Lines_C.end(), pred);
	    if (it == Lines_C.end())
		return -1;
	    return std::distance(Lines_C.begin(), it);
	}

	unsigned numLines() const { return Lines_C.size(); }

	vector<string>& lines() { return Lines_C; }
	const vector<string>& lines() const { return Lines_C; }

    protected:

	void removeLastIf(string& Text_Cr, char Char_cv) const;

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

	void setValue(const string& key, const string& value);
	bool getValue(const string& key, string& value) const;

	void setValue(const string& key, const vector<string>& values);
	bool getValue(const string& key, vector<string>& values) const;

    private:

	bool modified;

    };

}


#endif
