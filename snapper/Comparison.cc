/*
 * Copyright (c) [2011-2012] Novell, Inc.
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
#include "snapper/Log.h"
#include "snapper/File.h"
#include "snapper/Exception.h"


namespace snapper
{

    Comparison::Comparison(const Snapper* snapper, Snapshots::const_iterator snapshot1,
			   Snapshots::const_iterator snapshot2, bool delay)
	: snapper(snapper), snapshot1(snapshot1), snapshot2(snapshot2), initialized(false),
	  files(this)
    {
	if (snapshot1 == snapper->getSnapshots().end() ||
	    snapshot2 == snapper->getSnapshots().end() ||
	    snapshot1 == snapshot2)
	    throw IllegalSnapshotException();

	y2mil("num1:" << snapshot1->getNum() << " num2:" << snapshot2->getNum() << " delay:" <<
	      delay);

	if (!delay)
	{
	    files.initialize();
	    initialized = true;
	}
    }


    void
    Comparison::initialize()
    {
	if (!initialized)
	{
	    files.initialize();
	    initialized = true;
	}
    }


    Files&
    Comparison::getFiles()
    {
	if (!initialized)
	    throw;

	return files;
    }


    const Files&
    Comparison::getFiles() const
    {
	if (!initialized)
	    throw;

	return files;
    }


    UndoStatistic
    Comparison::getUndoStatistic() const
    {
	if (!initialized)
	    throw;

	return files.getUndoStatistic();
    }


    vector<UndoStep>
    Comparison::getUndoSteps() const
    {
	if (!initialized)
	    throw;

	return files.getUndoSteps();
    }


    bool
    Comparison::doUndoStep(const UndoStep& undo_step)
    {
	if (!initialized)
	    throw;

	return files.doUndoStep(undo_step);
    }


    bool
    Comparison::doUndo()
    {
	if (!initialized)
	    throw;

	return files.doUndo();
    }

}
