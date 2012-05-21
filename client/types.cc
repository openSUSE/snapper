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


#include "types.h"


XSnapshots::const_iterator
XSnapshots::findPre(const_iterator post) const
{
    if (post == entries.end() || post->isCurrent() || post->getType() != XPOST)
	throw;

    for (const_iterator it = begin(); it != end(); ++it)
    {
	if (it->getType() == XPRE && it->getNum() == post->getPreNum())
	    return it;
    }

    return end();
}


XSnapshots::const_iterator
XSnapshots::findPost(const_iterator pre) const
{
    if (pre == entries.end() || pre->isCurrent() || pre->getType() != XPRE)
	throw;

    for (const_iterator it = begin(); it != end(); ++it)
    {
	if (it->getType() == XPOST && it->getPreNum() == pre->getNum())
	    return it;
    }

    return end();
}


bool
XUndoStatistic::empty() const
{
    return numCreate == 0 && numModify == 0 && numDelete == 0;
}


namespace DBus
{
    const char* TypeInfo<XConfigInfo>::signature = "(ss)";
    const char* TypeInfo<XSnapshot>::signature = "(uquussa{ss})";
    const char* TypeInfo<XFile>::signature = "(ssb)";
    const char* TypeInfo<XUndo>::signature = "(sb)";


    Hihi&
    operator>>(Hihi& hihi, XConfigInfo& data)
    {
	hihi.open_recurse();
	hihi >> data.config_name >> data.subvolume;
	hihi.close_recurse();
	return hihi;
    }


    Hihi&
    operator>>(Hihi& hihi, XSnapshotType& data)
    {
	dbus_uint16_t tmp;
	hihi >> tmp;
	data = static_cast<XSnapshotType>(tmp);
	return hihi;
    }


    Hihi&
    operator>>(Hihi& hihi, XSnapshot& data)
    {
	hihi.open_recurse();
	hihi >> data.num >> data.type >> data.pre_num >> data.date >> data.description
	     >> data.cleanup >> data.userdata;
	hihi.close_recurse();
	return hihi;
    }


    Hihi&
    operator>>(Hihi& hihi, XFile& data)
    {
	hihi.open_recurse();
	hihi >> data.name >> data.status >> data.undo;
	hihi.close_recurse();
	return hihi;
    }


    Hoho&
    operator<<(Hoho& hoho, XSnapshotType data)
    {
	hoho << static_cast<dbus_uint16_t>(data);
	return hoho;
    }


    Hoho&
    operator<<(Hoho& hoho, const XUndo& data)
    {
	hoho.open_struct();
	hoho << data.name << data.undo;
	hoho.close_struct();
	return hoho;
    }

}
