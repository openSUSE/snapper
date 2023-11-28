/*
 * Copyright (c) [2012-2013] Novell, Inc.
 * Copyright (c) [2016-2023] SUSE LLC
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


#include "Types.h"


namespace DBus
{
    const char* TypeInfo<ConfigInfo>::signature = "(ssa{ss})";
    const char* TypeInfo<Snapshot>::signature = "(uquxussa{ss})";
    const char* TypeInfo<File>::signature = "(su)";
    const char* TypeInfo<QuotaData>::signature = "(tt)";
    const char* TypeInfo<FreeSpaceData>::signature = "(tt)";
    const char* TypeInfo<Plugins::Report::Entry>::signature = "(sasi)";


    Marshaller&
    operator<<(Marshaller& marshaller, const ConfigInfo& data)
    {
	marshaller.open_struct();
	marshaller << data.get_config_name() << data.get_subvolume() << data.get_all_values();
	marshaller.close_struct();
	return marshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, SnapshotType& data)
    {
	dbus_uint16_t tmp;
	unmarshaller >> tmp;
	data = static_cast<SnapshotType>(tmp);
	return unmarshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, SnapshotType data)
    {
	marshaller << static_cast<dbus_uint16_t>(data);
	return marshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const Snapshot& data)
    {
	marshaller.open_struct();
	marshaller << data.getNum() << data.getType() << data.getPreNum() << data.getDate()
	     << data.getUid() << data.getDescription() << data.getCleanup()
	     << data.getUserdata();
	marshaller.close_struct();
	return marshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const Snapshots& data)
    {
	marshaller.open_array(TypeInfo<Snapshot>::signature);
	for (Snapshots::const_iterator it = data.begin(); it != data.end(); ++it)
	    marshaller << *it;
	marshaller.close_array();
	return marshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const File& data)
    {
	marshaller.open_struct();
	marshaller << data.getName() << data.getPreToPostStatus();
	marshaller.close_struct();
	return marshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const QuotaData& data)
    {
	marshaller.open_struct();
	marshaller << data.size << data.used;
	marshaller.close_struct();
	return marshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const FreeSpaceData& data)
    {
	marshaller.open_struct();
	marshaller << data.size << data.free;
	marshaller.close_struct();
	return marshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const Files& data)
    {
	marshaller.open_array(TypeInfo<File>::signature);
	for (Files::const_iterator it = data.begin(); it != data.end(); ++it)
	    marshaller << *it;
	marshaller.close_array();
	return marshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, const Plugins::Report::Entry& data)
    {
	marshaller.open_struct();
	marshaller << data.name << data.args << data.exit_status;
	marshaller.close_struct();
	return marshaller;
    }

}
