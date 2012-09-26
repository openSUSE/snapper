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


#include <string>
#include <dbus/DBusNameBuilder.h>
#include <boost/algorithm/string/replace.hpp>

namespace DBus
{
   using std::string;
   
   string
   DBusNameBuilder::create_well_known_name(const string& unique_name)
   {
       if ((unique_name.length() < 4) ||
	   (unique_name.find_first_of('.', 0) == string::npos) ||
	   (unique_name[0] != ':'))
       {
	   throw DBusNameBuilderException();
       }

       string well_known_name = unique_name;

       // colon is illegal in bus names
       boost::algorithm::replace_all(well_known_name, ":", "s");
       // element name is not allowed to start with number
       boost::algorithm::replace_all(well_known_name, ".", "d");

       well_known_name.insert(0, CLIENT_PREFIX ".");

       return well_known_name;
   }
}