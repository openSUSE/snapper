/*
 * Copyright (c) [2011-2014] Novell, Inc.
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


#include <string>

#include <snapper/Snapper.h>


using namespace snapper;
using namespace std;


unsigned int
read_num(const string& str);

map<string, string>
read_userdata(const string& s, const map<string, string>& old = map<string, string>());

string
show_userdata(const map<string, string>& userdata);

map<string, string>
read_configdata(const list<string>& l, const map<string, string>& old = map<string, string>());

string
username(uid_t uid);


struct Differ
{
    Differ();

    void run(const string& f1, const string& f2) const;

    string command;
    string extensions;
};
