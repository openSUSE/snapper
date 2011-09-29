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
#include <errno.h>
#include <string.h>
#include <boost/algorithm/string.hpp>

#include "snapper/Snapshot.h"
#include "snapper/Snapper.h"
#include "snapper/AppUtil.h"
#include "snapper/XmlFile.h"
#include "snapper/Filesystem.h"
#include "snapper/Enum.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Exception.h"


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

	if (!snapshot.cleanup.empty())
	    s << " cleanup:\"" << snapshot.cleanup << "\"";

	if (!snapshot.userdata.empty())
	    s << " userdata:\"" << snapshot.userdata << "\"";

	return s;
    }


    // Directory where the info file is saved. Obviously not available for
    // current.
    // For btrfs e.g. "/.snapshots/1" or "/home/.snapshots/1".
    // For ext4 e.g. "/.snapshots-info/1" or "/home/.snapshots-info/1".
    string
    Snapshot::infoDir() const
    {
	if (isCurrent())
	    throw IllegalSnapshotException();

	return snapper->infosDir() + "/" + decString(num);
    }


    // Directory containing the actual content of the snapshot.
    // For btrfs e.g. "/" or "/home" for current and "/.snapshots/1/snapshot"
    // or "/home/.snapshots/1/snapshot" otherwise.
    // For ext4 e.g. "/" or "/home" for current and "/@1" or "/home@1"
    // otherwise.
    string
    Snapshot::snapshotDir() const
    {
	if (isCurrent())
	    return snapper->subvolumeDir();

	return snapper->getFilesystem()->snapshotDir(num);
    }


    void
    Snapshot::setDescription(const string& val)
    {
	if (isCurrent())
	    throw IllegalSnapshotException();

	description = val;
	info_modified = true;
    }


    void
    Snapshot::setCleanup(const string& val)
    {
	if (isCurrent())
	    throw IllegalSnapshotException();

	cleanup = val;
	info_modified = true;
    }


    void
    Snapshot::setUserdata(const map<string, string>& val)
    {
	if (isCurrent())
	    throw IllegalSnapshotException();

	for (map<string, string>::const_iterator it = val.begin(); it != val.end(); ++it)
	{
	    if (it->first.empty() || it->first.find_first_of(",=") != string::npos)
		throw InvalidUserdataException();

	    if (it->second.find_first_of(",=") != string::npos)
		throw InvalidUserdataException();
	}

	userdata = val;
	info_modified = true;
    }


    void
    Snapshots::read()
    {
	list<string> infos = glob(snapper->infosDir() + "/*/info.xml", GLOB_NOSORT);
	for (list<string>::const_iterator it1 = infos.begin(); it1 != infos.end(); ++it1)
	{
	    XmlFile file(*it1);
	    const xmlNode* root = file.getRootElement();
	    const xmlNode* node = getChildNode(root, "snapshot");

	    string tmp;

	    SnapshotType type;
	    if (!getChildValue(node, "type", tmp) || !toValue(tmp, type, true))
	    {
		y2err("type missing or invalid. not adding snapshot " << *it1);
		continue;
	    }

	    unsigned int num;
	    if (!getChildValue(node, "num", num) || num == 0)
	    {
		y2err("num missing or invalid. not adding snapshot " << *it1);
		continue;
	    }

	    time_t date;
	    if (!getChildValue(node, "date", tmp) || (date = scan_datetime(tmp, true)) == (time_t)(-1))
	    {
		y2err("date missing or invalid. not adding snapshot " << *it1);
		continue;
	    }

	    Snapshot snapshot(snapper, type, num, date);

	    it1->substr(snapper->infosDir().length() + 1) >> num;
	    if (num != snapshot.num)
	    {
		y2err("num mismatch. not adding snapshot " << *it1);
		continue;
	    }

	    getChildValue(node, "pre_num", snapshot.pre_num);

	    getChildValue(node, "description", snapshot.description);

	    getChildValue(node, "cleanup", snapshot.cleanup);

	    const list<const xmlNode*> l = getChildNodes(node, "userdata");
	    for (list<const xmlNode*>::const_iterator it2 = l.begin(); it2 != l.end(); ++it2)
	    {
		string key, value;
		getChildValue(*it2, "key", key);
		getChildValue(*it2, "value", value);
		if (!key.empty())
		    snapshot.userdata[key] = value;
	    }

	    if (!snapper->getFilesystem()->checkSnapshot(snapshot.num))
	    {
		y2err("snapshot check failed. not adding snapshot " << *it1);
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
	time_t t0 = time(NULL);
	time_t t1 = (time_t)(-1);

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

	    if (!i1->isCurrent())
	    {
		if (i1->date > t0)
		    y2err("snapshot num " << i1->num << " in future");

		if (t1 != (time_t)(-1) && i1->date < t1)
		    y2err("time shift detected at snapshot num " << i1->num);

		t1 = i1->date;
	    }
	}
    }


    void
    Snapshots::initialize()
    {
	entries.clear();

	Snapshot snapshot(snapper, SINGLE, 0, (time_t)(-1));
	snapshot.description = "current";
	entries.push_back(snapshot);

	read();

	check();
    }


    Snapshots::iterator
    Snapshots::findPost(const_iterator pre)
    {
	if (pre == entries.end() || pre->isCurrent() || pre->getType() != PRE)
	    throw IllegalSnapshotException();

	for (iterator it = begin(); it != end(); ++it)
	{
	    if (it->getType() == POST && it->getPreNum() == pre->getNum())
		return it;
	}

	return end();
    }


    Snapshots::const_iterator
    Snapshots::findPost(const_iterator pre) const
    {
	if (pre == entries.end() || pre->isCurrent() || pre->getType() != PRE)
	    throw IllegalSnapshotException();

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
	unsigned int num = entries.empty() ? 0 : entries.rbegin()->num;

	while (true)
	{
	    ++num;

	    if (snapper->getFilesystem()->checkSnapshot(num))
		continue;

	    if (mkdir((snapper->infosDir() + "/" + decString(num)).c_str(), 0777) == 0)
		break;

	    if (errno == EEXIST)
		continue;

	    y2err("mkdir failed errno:" << errno << " (" << strerror(errno) << ")");
	    throw IOErrorException();
	}

	return num;
    }


    void
    Snapshot::flushInfo()
    {
	if (!info_modified)
	    return;

	writeInfo();
	info_modified = false;
    }


    void
    Snapshot::writeInfo() const
    {
	XmlFile xml;
	xmlNode* node = xmlNewNode("snapshot");
	xml.setRootElement(node);

	setChildValue(node, "type", toString(type));

	setChildValue(node, "num", num);

	setChildValue(node, "date", datetime(date, true, true));

	if (type == POST)
	    setChildValue(node, "pre_num", pre_num);

	if (!description.empty())
	    setChildValue(node, "description", description);

	if (!cleanup.empty())
	    setChildValue(node, "cleanup", cleanup);

	for (map<string, string>::const_iterator it = userdata.begin(); it != userdata.end(); ++it)
	{
	    xmlNode* userdata_node = xmlNewChild(node, "userdata");
	    setChildValue(userdata_node, "key", it->first);
	    setChildValue(userdata_node, "value", it->second);
	}

	if (!xml.save(infoDir() + "/info.xml.tmp"))
	{
	    y2err("saving info.xml failed infoDir: " << infoDir() << " errno: << " << errno <<
		  " (" << strerror(errno) << ")");
	    throw IOErrorException();
	}

	if (rename(string(infoDir() + "/info.xml.tmp").c_str(), string(infoDir() + "/info.xml").c_str()) != 0)
	{
	    y2err("rename info.xml failed infoDir: " << infoDir() << " errno: << " << errno <<
		  " (" << strerror(errno) << ")");
	    throw IOErrorException();
	}
    }


    void
    Snapshot::mountFilesystemSnapshot() const
    {
	if (isCurrent())
	    throw IllegalSnapshotException();

	snapper->getFilesystem()->mountSnapshot(num);
    }


    void
    Snapshot::umountFilesystemSnapshot() const
    {
	if (isCurrent())
	    throw IllegalSnapshotException();

	snapper->getFilesystem()->umountSnapshot(num);
    }


    void
    Snapshot::createFilesystemSnapshot() const
    {
	if (isCurrent())
	    throw IllegalSnapshotException();

	snapper->getFilesystem()->createSnapshot(num);
    }


    void
    Snapshot::deleteFilesystemSnapshot() const
    {
	if (isCurrent())
	    throw IllegalSnapshotException();

	snapper->getFilesystem()->umountSnapshot(num);
	snapper->getFilesystem()->deleteSnapshot(num);
    }


    Snapshots::iterator
    Snapshots::createSingleSnapshot(string description)
    {
	Snapshot snapshot(snapper, SINGLE, nextNumber(), time(NULL));
	snapshot.description = description;
	snapshot.info_modified = true;

	return createHelper(snapshot);
    }


    Snapshots::iterator
    Snapshots::createPreSnapshot(string description)
    {
	Snapshot snapshot(snapper, PRE, nextNumber(), time(NULL));
	snapshot.description = description;
	snapshot.info_modified = true;

	return createHelper(snapshot);
    }


    Snapshots::iterator
    Snapshots::createPostSnapshot(string description, Snapshots::const_iterator pre)
    {
	if (pre == entries.end() || pre->isCurrent() || pre->getType() != PRE)
	    throw IllegalSnapshotException();

	Snapshot snapshot(snapper, POST, nextNumber(), time(NULL));
	snapshot.description = description;
	snapshot.pre_num = pre->getNum();
	snapshot.info_modified = true;

	return createHelper(snapshot);
    }


    Snapshots::iterator
    Snapshots::createHelper(Snapshot& snapshot)
    {
	try
	{
	    snapshot.createFilesystemSnapshot();
	}
	catch (const CreateSnapshotFailedException& e)
	{
	    rmdir(snapshot.infoDir().c_str());
	    throw;
	}

	try
	{
	    snapshot.flushInfo();
	}
	catch (const IOErrorException& e)
	{
	    snapshot.deleteFilesystemSnapshot();
	    rmdir(snapshot.infoDir().c_str());
	    throw;
	}

	return entries.insert(entries.end(), snapshot);
    }


    void
    Snapshots::deleteSnapshot(iterator snapshot)
    {
	if (snapshot == entries.end() || snapshot->isCurrent())
	    throw IllegalSnapshotException();

	snapshot->deleteFilesystemSnapshot();

	unlink((snapshot->infoDir() + "/info.xml").c_str());

	list<string> tmp1 = glob(snapshot->infoDir() + "/filelist-*.txt", GLOB_NOSORT);
	for (list<string>::const_iterator it = tmp1.begin(); it != tmp1.end(); ++it)
	    unlink(it->c_str());

	list<string> tmp2 = glob(snapper->infosDir() + "/*/filelist-" +
				 decString(snapshot->getNum()) + ".txt", GLOB_NOSORT);
	for (list<string>::const_iterator it = tmp2.begin(); it != tmp2.end(); ++it)
	    unlink(it->c_str());

	rmdir(snapshot->infoDir().c_str());

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
