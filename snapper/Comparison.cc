/*
 * Copyright (c) [2011-2014] Novell, Inc.
 * Copyright (c) 2022 SUSE LLC
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
#include <regex>

#include "snapper/Comparison.h"
#include "snapper/Snapper.h"
#include "snapper/Log.h"
#include "snapper/File.h"
#include "snapper/Exception.h"
#include "snapper/Compare.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/AsciiFile.h"
#include "snapper/Filesystem.h"
#include "snapper/ComparisonImpl.h"


namespace snapper
{
    using namespace std;


    Comparison::Comparison(const Snapper* snapper, Snapshots::const_iterator snapshot1,
			   Snapshots::const_iterator snapshot2, bool mount)
	: snapper(snapper), snapshot1(snapshot1), snapshot2(snapshot2), mount(mount),
	  files(&file_paths)
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

	if (mount)
	    do_mount();
    }


    Comparison::~Comparison()
    {
	if (mount)
	    do_umount();
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
    Comparison::do_mount() const
    {
	if (!getSnapshot1()->isCurrent())
	    getSnapshot1()->mountFilesystemSnapshot(false);
	if (!getSnapshot2()->isCurrent())
	    getSnapshot2()->mountFilesystemSnapshot(false);
    }


    void
    Comparison::do_umount() const
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

	files.clear();

	cmpdirs_cb_t cb = [this](const string& name, unsigned int status) {
	    files.push_back(File(&file_paths, name, status));
	};

	do_mount();

	{
	    SDir dir1 = getSnapshot1()->openSnapshotDir();
	    SDir dir2 = getSnapshot2()->openSnapshotDir();
	    snapper->getFilesystem()->cmpDirs(dir1, dir2, cb);
	}

	do_umount();

	files.sort();

	y2mil("found " << files.size() << " lines");
    }


    bool
    Comparison::check_header(const string& line) const
    {
	static const regex rx_header("snapper-([0-9\\.]+)-([a-z]+)-([0-9]+)-begin", regex::extended);

	smatch match;

	if (regex_match(line, match, rx_header))
	{
	    if (match[2] != "list" || match[3] != "1")
	    {
		y2err("unknown filelist format:'" << match[2] << "' version:'" << match[3] << "'");
		SN_THROW(Exception("header format/version not supported"));
	    }

	    return true;
	}
	else
	{
	    // fine, older files might not have a header

	    return false;
	}
    }


    bool
    Comparison::check_footer(const string& line) const
    {
	static const regex rx_footer("snapper-([0-9\\.]+)-([a-z]+)-([0-9]+)-end", regex::extended);

	return regex_match(line, rx_footer);
    }


    bool
    Comparison::load(int fd, Compression compression, bool invert)
    {
	files.clear();

	try
	{
	    AsciiFileReader ascii_file_reader(fd, compression);

	    bool has_header = false;
	    bool has_footer = false;

	    bool first = true;

	    string line;
	    while (ascii_file_reader.read_line(line))
	    {
		if (first)
		{
		    first = false;
		    if (check_header(line))
		    {
			has_header = true;
			continue;
		    }
		}
		else
		{
		    if (has_header && check_footer(line))
		    {
			has_footer = true;
			break;
		    }
		}

		string::size_type pos = line.find(" ");
		if (pos == string::npos)
		    SN_THROW(Exception("separator space not found"));

		unsigned int status = stringToStatus(string(line, 0, pos));
		string name = string(line, pos + 1);

		if (invert)
		    status = invertStatus(status);

		File file(&file_paths, name, status);
		files.push_back(file);
	    }

	    ascii_file_reader.close();

	    if (has_header && !has_footer)
		SN_THROW(Exception("footer not found"));

	    files.sort();

	    y2mil("read " << files.size() << " lines");

	    return true;
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    return false;
	}
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

	    string name = filelist_name(num1);

	    for (Compression compression : { Compression::GZIP, Compression::NONE })
	    {
		if (!is_available(compression))
		    continue;

		int fd = info_dir.open(add_extension(compression, name), O_RDONLY | O_NOATIME |
				       O_NOFOLLOW | O_CLOEXEC);
		if (fd > -1)
		{
		    if (load(fd, compression, invert))
			return true;
		}
	    }
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);
	}

	return false;
    }


    bool
    Comparison::save() const
    {
	y2mil("num1:" << getSnapshot1()->getNum() << " num2:" << getSnapshot2()->getNum());

	if (getSnapshot1()->isCurrent() || getSnapshot2()->isCurrent())
	    SN_THROW(IllegalSnapshotException());

	unsigned int num1 = getSnapshot1()->getNum();
	unsigned int num2 = getSnapshot2()->getNum();

	bool invert = num1 > num2;

	if (invert)
	    swap(num1, num2);

	Compression compression = snapper->get_compression();

	string file_name = add_extension(compression, filelist_name(num1));
	string tmp_name = file_name + ".tmp-XXXXXX";

	SDir info_dir = invert ? getSnapshot1()->openInfoDir() : getSnapshot2()->openInfoDir();

	int fd = info_dir.mktemp(tmp_name);
	if (fd < -1)
	    SN_THROW(IOErrorException(sformat("SDir::mktemp failed errno:%d (%s)", errno,
					      stringerror(errno).c_str())));

	try
	{
	    AsciiFileWriter ascii_file_writer(fd, compression);

	    ascii_file_writer.write_line("snapper-" VERSION "-list-1-begin");

	    for (const File& file : files)
	    {
		unsigned int status = file.getPreToPostStatus();

		if (invert)
		    status = invertStatus(status);

		string line = statusToString(status) + " " + file.getName();

		ascii_file_writer.write_line(line);
	    }

	    ascii_file_writer.write_line("snapper-" VERSION "-list-1-end");

	    ascii_file_writer.close();
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    info_dir.unlink(tmp_name, 0);

	    return false;
	}

	info_dir.rename(tmp_name, file_name);

	return true;
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
