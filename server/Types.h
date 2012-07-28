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
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>

#include <string>
#include <list>

#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/File.h>

using std::string;
using std::list;

using namespace snapper;

#include "dbus/DBusMessage.h"


struct Undo
{
    Undo() {}
    Undo(const string& filename, bool undo) : filename(filename), undo(undo) {}

    string filename;
    bool undo;
};


namespace DBus
{
    template <> struct TypeInfo<ConfigInfo> { static const char* signature; };
    template <> struct TypeInfo<Snapshot> { static const char* signature; };
    template <> struct TypeInfo<File> { static const char* signature; };
    template <> struct TypeInfo<Undo> { static const char* signature; };
    template <> struct TypeInfo<UndoStep> { static const char* signature; };

    Hoho& operator<<(Hoho& hoho, const ConfigInfo& data);

    Hihi& operator>>(Hihi& hihi, SnapshotType& data);
    Hoho& operator<<(Hoho& hoho, SnapshotType data);

    Hoho& operator<<(Hoho& hoho, const Snapshot& data);

    Hoho& operator<<(Hoho& hoho, const Snapshots& data);

    Hoho& operator<<(Hoho& hoho, const File& data);

    Hoho& operator<<(Hoho& hoho, const Files& data);

    Hihi& operator>>(Hihi& hihi, Undo& data);
    Hoho& operator<<(Hoho& hoho, const Undo& data);

    Hihi& operator>>(Hihi& hihi, UndoStep& data);
    Hoho& operator<<(Hoho& hoho, const UndoStep& data);

};
