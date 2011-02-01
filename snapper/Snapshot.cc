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


namespace snapper
{
    using std::list;


    vector<Snapshot>::const_iterator snapshot1;
    vector<Snapshot>::const_iterator snapshot2;

    Snapshots snapshots;


    std::ostream& operator<<(std::ostream& s, const Snapshot& x)
    {
	s << "type:" << toString(x.type) << " num:" << x.num;

	if (x.pre_num != 0)
	    s << " pre-num:" << x.pre_num;

	s << " date:" << x.date;

	if (!x.description.empty())
	    s << " description:" << x.description;

	return s;
    }


    string
    Snapshot::baseDir() const
    {
	assert(num != 0);

	return SNAPSHOTSDIR "/" + decString(num);
    }


    string
    Snapshot::snapshotDir() const
    {
	if (num == 0)
	    return "/";
	else
	    return baseDir() + "/snapshot";
    }


    void
    Snapshots::read()
    {
	list<string> infos = glob(SNAPSHOTSDIR "/*/info.xml", GLOB_NOSORT);
	for (list<string>::const_iterator it = infos.begin(); it != infos.end(); ++it)
	{
	    unsigned int num;
	    it->substr(11) >> num;

	    XmlFile file(*it);
	    const xmlNode* root = file.getRootElement();
	    const xmlNode* node = getChildNode(root, "snapshot");

	    Snapshot snapshot;

	    string tmp;

	    if (getChildValue(node, "type", tmp))
	    {
		if (!toValue(tmp, snapshot.type, true))
		{
		}
	    }

	    getChildValue(node, "num", snapshot.num);
	    assert(num == snapshot.num);

	    getChildValue(node, "date", snapshot.date);

	    getChildValue(node, "description", snapshot.description);

	    getChildValue(node, "pre_num", snapshot.pre_num);

	    entries.push_back(snapshot);
	}

	sort(entries.begin(), entries.end());
    }


    void
    Snapshots::initialize()
    {
	initialized = true;

	Snapshot snapshot;
	snapshot.type = SINGLE;
	snapshot.num = 0;
	snapshot.date = "now";
	snapshot.description = "current";
	entries.push_back(snapshot);

	read();
    }


    void
    Snapshots::assertInit()
    {
	if (!initialized)
	    initialize();
    }


    unsigned int
    Snapshots::nextNumber()
    {
	assertInit();

	unsigned int num = 1;

	if (!entries.empty())
	    num = entries.rbegin()->num + 1;

	mkdir((SNAPSHOTSDIR "/" + decString(num)).c_str(), 0777);

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

	setChildValue(node, "date", date);

	if (type == SINGLE || type == PRE)
	    setChildValue(node, "description", description);

	if (type == POST)
	    setChildValue(node, "pre_num", pre_num);

	xml.save(baseDir() + "/info.xml");

	return true;
    }


    bool
    Snapshot::createFilesystemSnapshot() const
    {
	SystemCmd cmd(BTRFSBIN " subvolume snapshot / " + snapshotDir());
	return cmd.retcode() == 0;
    }


    unsigned int
    Snapshots::createSingleSnapshot(string description)
    {
	Snapshot snapshot;
	snapshot.type = SINGLE;
	snapshot.num = nextNumber();
	snapshot.date = datetime();
	snapshot.description = description;

	entries.push_back(snapshot);

	snapshot.writeInfo();
	snapshot.createFilesystemSnapshot();

	return snapshot.num;
    }


    unsigned int
    Snapshots::createPreSnapshot(string description)
    {
	Snapshot snapshot;
	snapshot.type = PRE;
	snapshot.num = nextNumber();
	snapshot.date = datetime();
	snapshot.description = description;

	entries.push_back(snapshot);

	snapshot.writeInfo();
	snapshot.createFilesystemSnapshot();

	return snapshot.num;
    }


    unsigned int
    Snapshots::createPostSnapshot(unsigned int pre_num)
    {
	Snapshot snapshot;
	snapshot.type = POST;
	snapshot.num = nextNumber();
	snapshot.date = datetime();
	snapshot.pre_num = pre_num;

	entries.push_back(snapshot);

	snapshot.writeInfo();
	snapshot.createFilesystemSnapshot();

	return snapshot.num;
    }


    inline bool
    snapshot_num_less(const Snapshot& snapshot, unsigned int num)
    {
	return snapshot.num < num;
    }


    vector<Snapshot>::iterator
    Snapshots::find(unsigned int num)
    {
	return lower_bound(entries.begin(), entries.end(), num, snapshot_num_less);
    }


    vector<Snapshot>::const_iterator
    Snapshots::find(unsigned int num) const
    {
	return lower_bound(entries.begin(), entries.end(), num, snapshot_num_less);
    }

}
