/*
 * Copyright (c) 2012 Novell, Inc.
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#include <string>
#include <list>
#include <map>

using std::string;
using std::list;
using std::map;

#include "dbus/DBusConnection.h"
#include "dbus/DBusMessage.h"


struct XConfigInfo
{
    string config_name;
    string subvolume;
};


enum XSnapshotType { XSINGLE, XPRE, XPOST };


struct XSnapshot
{
    XSnapshotType type;
    unsigned int num;
    time_t date;
    unsigned int pre_num;
    string description;
    string cleanup;
    map<string, string> userdata;
};


struct XFile
{
    string status;
    string filename;
    bool undo;
};


struct XUndo
{
    string filename;
    bool undo;
};


namespace DBus
{
    
    template <> struct TypeInfo<XSnapshot> { static const char* signature; };
    template <> struct TypeInfo<XConfigInfo> { static const char* signature; };
    template <> struct TypeInfo<XFile> { static const char* signature; };
    template <> struct TypeInfo<XUndo> { static const char* signature; };
   
    Hihi& operator>>(Hihi& hihi, XConfigInfo& data);
    Hihi& operator>>(Hihi& hihi, XSnapshotType& data);
    Hihi& operator>>(Hihi& hihi, XSnapshot& data);
    Hihi& operator>>(Hihi& hihi, XFile& data);

    Hoho& operator<<(Hoho& hoho, XSnapshotType data);
    Hoho& operator<<(Hoho& hoho, const XUndo& data);

};

