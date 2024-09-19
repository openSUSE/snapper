/*
 * Copyright (c) 2012 Novell, Inc.
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


#include <snapper/SnapperTmpl.h>
#include <snapper/Exception.h>

#include "types.h"


namespace DBus
{
    const char* TypeInfo<XConfigInfo>::signature = "(ssa{ss})";
    const char* TypeInfo<XSnapshot>::signature = "(uquxussa{ss})";
    const char* TypeInfo<XFile>::signature = "(su)";
    const char* TypeInfo<XReport>::signature = "(sasi)";


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, XConfigInfo& data)
    {
	unmarshaller.open_recurse();
	unmarshaller >> data.config_name >> data.subvolume >> data.raw;
	unmarshaller.close_recurse();
	return unmarshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, SnapshotType& data)
    {
	dbus_uint16_t tmp;
	unmarshaller >> tmp;
	data = static_cast<SnapshotType>(tmp);
	return unmarshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, XSnapshot& data)
    {
	unmarshaller.open_recurse();
	unmarshaller >> data.num >> data.type >> data.pre_num >> data.date >> data.uid >> data.description
		     >> data.cleanup >> data.userdata;
	unmarshaller.close_recurse();
	return unmarshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, XFile& data)
    {
	unmarshaller.open_recurse();
	unmarshaller >> data.name >> data.status;
	unmarshaller.close_recurse();
	return unmarshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, QuotaData& data)
    {
	unmarshaller.open_recurse();
	unmarshaller >> data.size >> data.used;
	unmarshaller.close_recurse();
	return unmarshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, FreeSpaceData& data)
    {
	unmarshaller.open_recurse();
	unmarshaller >> data.size >> data.free;
	unmarshaller.close_recurse();
	return unmarshaller;
    }


    Marshaller&
    operator<<(Marshaller& marshaller, SnapshotType data)
    {
	marshaller << static_cast<dbus_uint16_t>(data);
	return marshaller;
    }


    Unmarshaller&
    operator>>(Unmarshaller& unmarshaller, XReport& data)
    {
	unmarshaller.open_recurse();
	unmarshaller >> data.name >> data.args >> data.exit_status;
	unmarshaller.close_recurse();
	return unmarshaller;
    }

}
