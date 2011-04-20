/*
 * Copyright (c) 2011 Novell, Inc.
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


#ifndef COMPARE_H
#define COMPARE_H


#include <string>
#include <functional>


namespace snapper
{
    using std::string;


    struct IOErrorException : public std::exception
    {
	explicit IOErrorException() throw() {}
	virtual const char* what() const throw() { return "IO error"; }
    };


    typedef std::function<void(const string& name, unsigned int status)> cmpdirs_cb_t;


    unsigned int
    cmpFiles(const string& name1, const string& name2);

    void
    cmpDirs(const string& path1, const string& path2, cmpdirs_cb_t cb);

    string
    statusToString(unsigned int status);

    unsigned int
    stringToStatus(const string& str);

    unsigned int
    invertStatus(unsigned int status);

}


#endif
