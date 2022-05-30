/*
 * Copyright (c) 2012 Novell, Inc.
 * Copyright (c) [2016,2018] SUSE LLC
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

#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/File.h>
#include <dbus/DBusMessage.h>


using std::string;

using namespace snapper;


namespace DBus
{
    template <> struct TypeInfo<ConfigInfo> { static const char* signature; };
    template <> struct TypeInfo<Snapshot> { static const char* signature; };
    template <> struct TypeInfo<File> { static const char* signature; };
    template <> struct TypeInfo<QuotaData> { static const char* signature; };
    template <> struct TypeInfo<FreeSpaceData> { static const char* signature; };

    Marshaller& operator<<(Marshaller& marshaller, const ConfigInfo& data);

    Unmarshaller& operator>>(Unmarshaller& unmarshaller, SnapshotType& data);
    Marshaller& operator<<(Marshaller& marshaller, SnapshotType data);

    Marshaller& operator<<(Marshaller& marshaller, const Snapshot& data);

    Marshaller& operator<<(Marshaller& marshaller, const Snapshots& data);

    Marshaller& operator<<(Marshaller& marshaller, const File& data);

    Marshaller& operator<<(Marshaller& marshaller, const Files& data);

    Marshaller& operator<<(Marshaller& marshaller, const QuotaData& data);

    Marshaller& operator<<(Marshaller& marshaller, const FreeSpaceData& data);

}
