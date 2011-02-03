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


#include <sys/stat.h>
#include <sys/types.h>
#include <glob.h>
#include <string.h>

#include "snapper/Snapper.h"
#include "snapper/AppUtil.h"
#include "snapper/XmlFile.h"
#include "snapper/Enum.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/File.h"


namespace snapper
{
    using namespace std;


    void
    startBackgroundComparsion(list<Snapshot>::const_iterator snapshot1,
			      list<Snapshot>::const_iterator snapshot2)
    {
	y2mil("num1:" << snapshot1->getNum() << " num2:" << snapshot2->getNum());

	string dir1 = snapshot1->snapshotDir();
	string dir2 = snapshot2->snapshotDir();

	string output = snapshot2->baseDir() + "/filelist-" + decString(snapshot1->getNum()) +
	    ".txt";

	SystemCmd(COMPAREDIRSBIN " " + quote(dir1) + " " + quote(dir2) + " " + quote(output));
    }


    bool
    setComparisonNums(list<Snapshot>::const_iterator new_snapshot1,
		      list<Snapshot>::const_iterator new_snapshot2)
    {
	y2mil("num1:" << new_snapshot1->getNum() << " num2:" << new_snapshot2->getNum());

	snapshot1 = new_snapshot1;
	snapshot2 = new_snapshot2;

	files.assertInit();

	return true;
    }


    CompareCallback* compare_callback = NULL;

    void
    setCompareCallback(CompareCallback* p)
    {
	compare_callback = p;
    }


}
