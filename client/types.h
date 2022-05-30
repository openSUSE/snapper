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


#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

#include "dbus/DBusMessage.h"
#include "dbus/DBusConnection.h"
#include "snapper/Snapshot.h"
#include "snapper/File.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/Snapper.h"

using namespace snapper;


struct XConfigInfo
{
    string config_name;
    string subvolume;

    map<string, string> raw;
};


struct XSnapshot
{
    SnapshotType getType() const { return type; }

    unsigned int getNum() const { return num; }

    time_t getDate() const { return date; }

    uid_t getUid() const { return uid; }

    unsigned int getPreNum() const { return pre_num; }

    const string& getDescription() const { return description; }

    const string& getCleanup() const { return cleanup; }

    const map<string, string>& getUserdata() const { return userdata; }

    SnapshotType type;
    unsigned int num;
    time_t date;
    uid_t uid;
    unsigned int pre_num;
    string description;
    string cleanup;
    map<string, string> userdata;
};


struct XSnapshots
{
    typedef vector<XSnapshot>::const_iterator const_iterator;

    const_iterator begin() const { return entries.begin(); }
    const_iterator end() const { return entries.end(); }

    vector<XSnapshot> entries;
};


struct XFile
{
    string name;
    unsigned int status;
};


namespace DBus
{

    template <> struct TypeInfo<XSnapshot> { static const char* signature; };
    template <> struct TypeInfo<XConfigInfo> { static const char* signature; };
    template <> struct TypeInfo<XFile> { static const char* signature; };

    Unmarshaller& operator>>(Unmarshaller& unmarshaller, XConfigInfo& data);

    Unmarshaller& operator>>(Unmarshaller& unmarshaller, SnapshotType& data);
    Marshaller& operator<<(Marshaller& marshaller, SnapshotType data);

    Unmarshaller& operator>>(Unmarshaller& unmarshaller, XSnapshot& data);

    Unmarshaller& operator>>(Unmarshaller& unmarshaller, XFile& data);

    Unmarshaller& operator>>(Unmarshaller& unmarshaller, QuotaData& data);

    Unmarshaller& operator>>(Unmarshaller& unmarshaller, FreeSpaceData& data);

}
