/*
 * Copyright (c) [2011-2014] Novell, Inc.
 * Copyright (c) 2018 SUSE LLC
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

#include "dbus/DBusConnection.h"


namespace snapper
{

    using std::string;


/**
 * Translate an DBus exception to the corresponding snapper exception
 * (iff such exists). Unfinished.
 */
void
convert_exception(const DBus::ErrorException& e) __attribute__ ((__noreturn__));


/**
 * Returns a string explaining the DBus exception. Function should
 * likely be removed once convert_exception is complete and used
 * everywhere.
 */
string
error_description(const DBus::ErrorException& e);

}
