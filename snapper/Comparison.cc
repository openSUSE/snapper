/*
 * Copyright (c) [2011-2014] Novell, Inc.
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


#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "snapper/Comparison.h"
#include "snapper/Snapper.h"
#include "snapper/Log.h"
#include "snapper/File.h"
#include "snapper/Exception.h"
#include "snapper/Compare.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/AsciiFile.h"
#include "snapper/Filesystem.h"


namespace snapper
{
    using namespace std;


    Comparison::Comparison(const Snapper* snapper, Snapshots::const_iterator snapshot1,
			   Snapshots::const_iterator snapshot2)
	: snapper(snapper), snapshot1(snapshot1), snapshot2(snapshot2), files(&file_paths)
    {
	if (snapshot1 == snapper->getSnapshots().end() ||
	    snapshot2 == snapper->getSnapshots().end() ||
	    snapshot1 == snapshot2)
	    SN_THROW(IllegalSnapshotException());

	y2mil("num1:" << snapshot1->getNum() << " num2:" << snapshot2->getNum());

	file_paths.system_path = snapper->subvolumeDir();
	file_paths.pre_path = snapshot1->snapshotDir();
	file_paths.post_path = snapshot2->snapshotDir();

	initialize();
    }


    void
    Comparison::initialize()
    {
	// When booting a snapshot the current snapshot could be read-only.
	// But which snapshot is booted as current snapshot might not be constant.

	bool fixed = !getSnapshot1()->isCurrent() && !getSnapshot2()->isCurrent();

	if (fixed)
	{
	    try
	    {
		fixed = getSnapshot1()->isReadOnly() && getSnapshot2()->isReadOnly();
	    }
	    catch (const runtime_error& e)
	    {
		y2err("failed to query read-only status, " << e.what());
		fixed = false;
	    }
	}

	if (!fixed)
	{
	    create();
	}
	else
	{
	    if (!load())
	    {
		create();
		save();
	    }
	}

	filter();
    }


    void
    Comparison::mount() const
    {
	if (!getSnapshot1()->isCurrent())
	    getSnapshot1()->mountFilesystemSnapshot(false);
	if (!getSnapshot2()->isCurrent())
	    getSnapshot2()->mountFilesystemSnapshot(false);
    }


    void
    Comparison::umount() const
    {
	if (!getSnapshot1()->isCurrent())
	    getSnapshot1()->umountFilesystemSnapshot(false);
	if (!getSnapshot2()->isCurrent())
	    getSnapshot2()->umountFilesystemSnapshot(false);
    }


    void
    Comparison::create()
    {
	y2mil("num1:" << getSnapshot1()->getNum() << " num2:" << getSnapshot2()->getNum());

	cmpdirs_cb_t cb = [this](const string& name, unsigned int status) {
	    files.push_back(File(&file_paths, name, status));
	};

	mount();

	{
	    SDir dir1 = getSnapshot1()->openSnapshotDir();
	    SDir dir2 = getSnapshot2()->openSnapshotDir();
	    snapper->getFilesystem()->cmpDirs(dir1, dir2, cb);
	}

	umount();

	files.sort();

	y2mil("found " << files.size() << " lines");
    }


    bool
    Comparison::load()
    {
	y2mil("num1:" << getSnapshot1()->getNum() << " num2:" << getSnapshot2()->getNum());

	if (getSnapshot1()->isCurrent() || getSnapshot2()->isCurrent())
	    SN_THROW(IllegalSnapshotException());

	unsigned int num1 = getSnapshot1()->getNum();
	unsigned int num2 = getSnapshot2()->getNum();

	bool invert = num1 > num2;

	if (invert)
	    swap(num1, num2);

	try
	{
	    SDir infos_dir = getSnapper()->openInfosDir();
	    SDir info_dir = SDir(infos_dir, decString(num2));

	    int fd = info_dir.open("filelist-" + decString(num1) + ".txt", O_RDONLY | O_NOATIME |
				   O_NOFOLLOW | O_CLOEXEC);
	    if (fd == -1)
		return false;

	    AsciiFileReader asciifile(fd);

	    string line;
	    while (asciifile.getline(line))
	    {
		string::size_type pos = line.find(" ");
		if (pos == string::npos)
		    continue;

		unsigned int status = stringToStatus(string(line, 0, pos));
		string name = string(line, pos + 1);

		if (invert)
		    status = invertStatus(status);

		File file(&file_paths, name, status);
		files.push_back(file);
	    }
	}
	catch (const FileNotFoundException& e)
	{
	    return false;
	}

	files.sort();

	y2mil("read " << files.size() << " lines");

	return true;
    }


    void
    Comparison::save()
    {
	y2mil("num1:" << getSnapshot1()->getNum() << " num2:" << getSnapshot2()->getNum());

	if (getSnapshot1()->isCurrent() || getSnapshot2()->isCurrent())
	    SN_THROW(IllegalSnapshotException());

	unsigned int num1 = getSnapshot1()->getNum();
	unsigned int num2 = getSnapshot2()->getNum();

	bool invert = num1 > num2;

	if (invert)
	    swap(num1, num2);

	string file_name = "filelist-" + decString(num1) + ".txt";
	string tmp_name = file_name + ".tmp-XXXXXX";

	SDir info_dir = invert ? getSnapshot1()->openInfoDir() : getSnapshot2()->openInfoDir();

	FILE* file = fdopen(info_dir.mktemp(tmp_name), "w");
	if (!file)
	    SN_THROW(IOErrorException(sformat("mkstemp failed errno:%d (%s)", errno,
					      stringerror(errno).c_str())));

	for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	{
	    unsigned int status = it->getPreToPostStatus();

	    if (invert)
		status = invertStatus(status);

	    fprintf(file, "%s %s\n", statusToString(status).c_str(), it->getName().c_str());
	}

	fclose(file);

	info_dir.rename(tmp_name, file_name);
    }


    void
    Comparison::filter()
    {
	const vector<string>& ignore_patterns = getSnapper()->getIgnorePatterns();
	files.filter(ignore_patterns);
    }


    UndoStatistic
    Comparison::getUndoStatistic() const
    {
	if (getSnapshot1()->isCurrent())
	    SN_THROW(IllegalSnapshotException());

	return files.getUndoStatistic();
    }


    XAUndoStatistic
    Comparison::getXAUndoStatistic() const
    {
        if (getSnapshot1()->isCurrent())
            SN_THROW(IllegalSnapshotException());

        return files.getXAUndoStatistic();
    }


    vector<UndoStep>
    Comparison::getUndoSteps() const
    {
	if (getSnapshot1()->isCurrent())
	    SN_THROW(IllegalSnapshotException());

	return files.getUndoSteps();
    }


    bool
    Comparison::doUndoStep(const UndoStep& undo_step)
    {
	if (getSnapshot1()->isCurrent())
	    SN_THROW(IllegalSnapshotException());

	return files.doUndoStep(undo_step);
    }

}
