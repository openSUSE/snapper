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


#include "snapper/Comparison.h"
#include "snapper/Snapper.h"
#include "snapper/AppUtil.h"
#include "snapper/File.h"
#include "snapper/Exception.h"


namespace snapper
{

    Comparison::Comparison(const Snapper* snapper, Snapshots::const_iterator snapshot1,
			   Snapshots::const_iterator snapshot2)
	: snapper(snapper), snapshot1(snapshot1), snapshot2(snapshot2), files(this)
    {
	if (snapshot1 == snapper->getSnapshots().end() ||
	    snapshot2 == snapper->getSnapshots().end() ||
	    snapshot1 == snapshot2)
	    throw IllegalSnapshotException();

	y2mil("num1:" << snapshot1->getNum() << " num2:" << snapshot2->getNum());

	files.initialize();
    }


    RollbackStatistic
    Comparison::getRollbackStatistic() const
    {
	return files.getRollbackStatistic();
    }


    bool
    Comparison::doRollback()
    {
	return files.doRollback();
    }

}
