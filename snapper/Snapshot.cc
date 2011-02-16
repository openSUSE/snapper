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
#include <boost/algorithm/string.hpp>

#include "snapper/Snapshot.h"
#include "snapper/Snapper.h"
#include "snapper/AppUtil.h"
#include "snapper/XmlFile.h"
#include "snapper/Enum.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"


namespace snapper
{
    using std::list;


    std::ostream& operator<<(std::ostream& s, const Snapshot& snapshot)
    {
	s << "type:" << toString(snapshot.type) << " num:" << snapshot.num;

	if (snapshot.pre_num != 0)
	    s << " pre-num:" << snapshot.pre_num;

	s << " date:\"" << datetime(snapshot.date, true, true) << "\"";

	if (!snapshot.description.empty())
	    s << " description:\"" << snapshot.description << "\"";

	return s;
    }


    // Directory where the info file is saved, e.g. "/snapshots/1" or
    // "/home/snapshots/1". Obviously not available for current.
    string
    Snapshot::baseDir() const
    {
	assert(num != 0);

	return snapper->snapshotsDir() + "/" + decString(num);
    }


    // Directory containing the actual snapshot, e.g. "/" or "/home" for
    // current and "/snapshots/1/snapshot" or "/home/snapshots/1/snapshot"
    // otherwise.
    string
    Snapshot::snapshotDir() const
    {
	if (num == 0)
	    return snapper->rootDir();
	else
	    return baseDir() + SNAPSHOTDIR;
    }


    void
    Snapshot::setDescription(const string& desc)
    {
	description = desc;
	writeInfo();
    }


    void
    Snapshots::read()
    {
	list<string> infos = glob(snapper->snapshotsDir() + "/*/info.xml", GLOB_NOSORT);
	for (list<string>::const_iterator it = infos.begin(); it != infos.end(); ++it)
	{
	    unsigned int num;
	    it->substr(snapper->snapshotsDir().length() + 1) >> num;

	    XmlFile file(*it);
	    const xmlNode* root = file.getRootElement();
	    const xmlNode* node = getChildNode(root, "snapshot");

	    Snapshot snapshot(snapper);

	    string tmp;

	    if (getChildValue(node, "type", tmp))
	    {
		if (!toValue(tmp, snapshot.type, true))
		{
		}
	    }

	    getChildValue(node, "num", snapshot.num);
	    assert(num == snapshot.num);

	    if (getChildValue(node, "date", tmp))
	    {
		// TODO: remove someday
		if (boost::ends_with(tmp, " GMT"))
		    tmp.erase(19);

		assert(tmp.size() == 19);

		snapshot.date = scan_datetime(tmp, true);
	    }

	    getChildValue(node, "description", snapshot.description);

	    getChildValue(node, "pre_num", snapshot.pre_num);

	    if (!checkDir(snapshot.snapshotDir()))
	    {
		y2err("snapshot directory does not exist. not adding snapshot " << num);
		continue;
	    }

	    entries.push_back(snapshot);
	}

	entries.sort();

	y2mil("found " << entries.size() << " snapshots");
    }


    void
    Snapshots::check() const
    {
	for (const_iterator i1 = begin(); i1 != end(); ++i1)
	{
	    switch (i1->type)
	    {
		case SINGLE:
		{
		}
		break;

		case PRE:
		{
		    int n = 0;
		    for (const_iterator i2 = begin(); i2 != end(); ++i2)
			if (i2->pre_num == i1->num)
			    n++;
		    if (n > 1)
			y2err("pre-num " << i1->num << " has " << n << " post-nums");
		}
		break;

		case POST:
		{
		    if (i1->pre_num > i1->num)
			y2err("pre-num " << i1->pre_num << " larger than post-num " << i1->num);

		    const_iterator i2 = find(i1->pre_num);
		    if (i2 == end())
			y2err("pre-num " << i1->pre_num << " for post-num " << i1->num <<
			      " does not exist");
		    else
			if (i2->type != PRE)
			    y2err("pre-num " << i1->pre_num << " for post-num " << i1->num <<
				  " is of type " << toString(i2->type));
		}
		break;
	    }
	}
    }


    void
    Snapshots::initialize()
    {
	entries.clear();

	Snapshot snapshot(snapper);
	snapshot.type = SINGLE;
	snapshot.num = 0;
	snapshot.date = (time_t)(-1);
	snapshot.description = "current";
	entries.push_back(snapshot);

	read();

	check();
    }


    Snapshots::const_iterator
    Snapshots::findPost(const_iterator pre) const
    {
	assert(pre != end());
	assert(pre->getType() == PRE);

	for (const_iterator it = begin(); it != end(); ++it)
	{
	    if (it->getType() == POST && it->getPreNum() == pre->getNum())
		return it;
	}

	return end();
    }


    unsigned int
    Snapshots::nextNumber()
    {
	unsigned int num = 1;

	if (!entries.empty())
	    num = entries.rbegin()->num + 1;

	mkdir((snapper->snapshotsDir() + "/" + decString(num)).c_str(), 0777);

	// TODO check EEXIST

	return num;
    }


    bool
    Snapshot::writeInfo() const
    {
	XmlFile xml;
	xmlNode* node = xmlNewNode("snapshot");
	xml.setRootElement(node);

	setChildValue(node, "type", toString(type));

	setChildValue(node, "num", num);

	setChildValue(node, "date", datetime(date, true, true));

	if (!description.empty())
	    setChildValue(node, "description", description);

	if (type == POST)
	    setChildValue(node, "pre_num", pre_num);

	xml.save(baseDir() + "/info.xml");

	return true;
    }


    bool
    Snapshot::createFilesystemSnapshot() const
    {
	SystemCmd cmd(BTRFSBIN " subvolume snapshot " + snapper->rootDir() + " " + snapshotDir());
	return cmd.retcode() == 0;
    }


    bool
    Snapshot::deleteFilesystemSnapshot() const
    {
	SystemCmd cmd(BTRFSBIN " subvolume delete /" + snapshotDir());
	return cmd.retcode() == 0;
    }


    Snapshots::iterator
    Snapshots::createSingleSnapshot(string description)
    {
	Snapshot snapshot(snapper);
	snapshot.type = SINGLE;
	snapshot.num = nextNumber();
	snapshot.date = time(NULL);
	snapshot.description = description;

	snapshot.writeInfo();
	snapshot.createFilesystemSnapshot();

	return entries.insert(entries.end(), snapshot);
    }


    Snapshots::iterator
    Snapshots::createPreSnapshot(string description)
    {
	Snapshot snapshot(snapper);
	snapshot.type = PRE;
	snapshot.num = nextNumber();
	snapshot.date = time(NULL);
	snapshot.description = description;

	snapshot.writeInfo();
	snapshot.createFilesystemSnapshot();

	return entries.insert(entries.end(), snapshot);
    }


    Snapshots::iterator
    Snapshots::createPostSnapshot(Snapshots::const_iterator pre)
    {
	Snapshot snapshot(snapper);
	snapshot.type = POST;
	snapshot.num = nextNumber();
	snapshot.date = time(NULL);
	snapshot.pre_num = pre->getNum();

	snapshot.writeInfo();
	snapshot.createFilesystemSnapshot();

	return entries.insert(entries.end(), snapshot);
    }


    void
    Snapshots::deleteSnapshot(iterator snapshot)
    {
	assert(!snapshot->isCurrent());

	snapshot->deleteFilesystemSnapshot();

	unlink((snapshot->baseDir() + "/info.xml").c_str());

	list<string> tmp1 = glob(snapshot->baseDir() + "/filelist-*.txt", GLOB_NOSORT);
	for (list<string>::const_iterator it = tmp1.begin(); it != tmp1.end(); ++it)
	    unlink(it->c_str());

	list<string> tmp2 = glob(snapper->snapshotsDir() + "/*/filelist-" +
				 decString(snapshot->getNum()) + ".txt", GLOB_NOSORT);
	for (list<string>::const_iterator it = tmp2.begin(); it != tmp2.end(); ++it)
	    unlink(it->c_str());

	rmdir(snapshot->baseDir().c_str());

	entries.erase(snapshot);
    }


    struct num_is
    {
        num_is(unsigned int num) : num(num) {}
        bool operator()(const Snapshot& s) const { return s.getNum() == num; }
        const unsigned int num;
    };


    Snapshots::iterator
    Snapshots::find(unsigned int num)
    {
	return find_if(entries.begin(), entries.end(), num_is(num));
    }


    Snapshots::const_iterator
    Snapshots::find(unsigned int num) const
    {
	return find_if(entries.begin(), entries.end(), num_is(num));
    }

}
