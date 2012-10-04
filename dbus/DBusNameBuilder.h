/*
 * Copyright (c) [2011-2012] Novell, Inc.
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


#ifndef SNAPPER_CLIENTNAMEBUILDER_NAME_H
#define SNAPPER_CLIENTNAMEBUILDER_NAME_H

#define CLIENT_PREFIX "org.opensuse.Snapper.Client"

namespace DBus
{
    using std::string;
    
    class DBusNameBuilder
    {
    private:
	DBusNameBuilder() {};
	~DBusNameBuilder() {};
    public:
	static string create_well_known_name(const string &unique_name);
    };
    
    struct DBusNameBuilderException : public std::exception
    {
	explicit DBusNameBuilderException() throw() {}
	virtual const char* what() const throw() { return "invalid DBus unique name"; }
    };
}

#endif