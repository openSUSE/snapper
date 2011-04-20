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


#include <unistd.h>
#include <fstream>

#include "snapper/AppUtil.h"
#include "snapper/AsciiFile.h"
#include "snapper/SnapperTypes.h"
#include "snapper/Exception.h"


namespace snapper
{
    using namespace std;


    AsciiFileReader::AsciiFileReader(FILE* file)
	: file(file), buffer(NULL), len(0)
    {
	if (file == NULL)
	{
	    y2err("file is NULL");
	    throw FileNotFoundException();
	}
    }


    AsciiFileReader::AsciiFileReader(const string& filename)
	: file(NULL), buffer(NULL), len(0)
    {
	file = fopen(filename.c_str(), "r");
	if (file == NULL)
	{
	    y2err("open for '" << filename << "' failed");
	    throw FileNotFoundException();
	}
    }


    AsciiFileReader::~AsciiFileReader()
    {
	free(buffer);
	fclose(file);
    }


    bool
    AsciiFileReader::getline(string& line)
    {
	ssize_t n = ::getline(&buffer, &len, file);
	if (n == -1)
	    return false;

	if (buffer[n - 1] != '\n')
	    line = string(buffer, 0, n);
	else
	    line = string(buffer, 0, n - 1);

	return true;
    }


AsciiFile::AsciiFile(const char* Name_Cv, bool remove_empty)
    : Name_C(Name_Cv),
      remove_empty(remove_empty)
{
    reload();
}


AsciiFile::AsciiFile(const string& Name_Cv, bool remove_empty)
    : Name_C(Name_Cv),
      remove_empty(remove_empty)
{
    reload();
}


    void
    AsciiFile::reload()
    {
	y2mil("loading file " << Name_C);
	clear();

	AsciiFileReader file(Name_C);

	string line;
	while (file.getline(line))
	    Lines_C.push_back(line);
    }


bool
AsciiFile::save()
{
    if (remove_empty && Lines_C.empty())
    {
	y2mil("deleting file " << Name_C);

	if (access(Name_C.c_str(), F_OK) != 0)
	    return true;

	return unlink(Name_C.c_str()) == 0;
    }
    else
    {
	y2mil("saving file " << Name_C);

	ofstream file( Name_C.c_str() );
	classic(file);

	for (vector<string>::const_iterator it = Lines_C.begin(); it != Lines_C.end(); ++it)
	    file << *it << std::endl;

	file.close();

	return file.good();
    }
}


    void
    AsciiFile::logContent() const
    {
	y2mil("content of " << (Name_C.empty() ? "<nameless>" : Name_C));
	for (vector<string>::const_iterator it = Lines_C.begin(); it != Lines_C.end(); ++it)
	    y2mil(*it);
    }


    void
    SysconfigFile::setValue(const string& key, const string& value)
    {
	string line = key + "=\"" + value + "\"";

	Regex rx('^' + Regex::ws + key + '=' + "(['\"]?)([^'\"]*)\\1" + Regex::ws + '$');

	vector<string>::iterator it = find_if(lines(), regex_matches(rx));
	if (it == lines().end())
	    push_back(line);
	else
	    *it = line;

	modified = true;
    }


    bool
    SysconfigFile::getValue(const string& key, string& value) const
    {
	Regex rx('^' + Regex::ws + key + '=' + "(['\"]?)([^'\"]*)\\1" + Regex::ws + '$');

	if (find_if(lines(), regex_matches(rx)) == lines().end())
	    return false;

	value = rx.cap(2);
	y2mil("key:" << key << " value:" << value);
	return true;
    }


    void
    SysconfigFile::setValue(const string& key, const vector<string>& values)
    {
	setValue(key, boost::join(values, " "));
    }


    bool
    SysconfigFile::getValue(const string& key, vector<string>& values) const
    {
	string tmp;
	if (!getValue("SNAPPER_CONFIGS", tmp))
	    return false;

	boost::split(values, tmp, boost::is_any_of(" \t"));
	return true;
    }

}
