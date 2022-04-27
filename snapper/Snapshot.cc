/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2022] SUSE LLC
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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <regex>
#include <boost/algorithm/string.hpp>

#include "snapper/Snapshot.h"
#include "snapper/Snapper.h"
#include "snapper/AppUtil.h"
#include "snapper/XmlFile.h"
#include "snapper/Filesystem.h"
#ifdef ENABLE_BTRFS
#include "snapper/Btrfs.h"
#endif
#include "snapper/Enum.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Exception.h"
#include "snapper/Hooks.h"
#include "snapper/ComparisonImpl.h"


namespace snapper
{
    using std::list;


    std::ostream& operator<<(std::ostream& s, const Snapshot& snapshot)
    {
	s << "type:" << toString(snapshot.type) << " num:" << snapshot.num;

	if (snapshot.pre_num != 0)
	    s << " pre-num:" << snapshot.pre_num;

	s << " date:\"" << datetime(snapshot.date, true, true) << "\"";

	if (snapshot.uid != 0)
	    s << "uid:" << snapshot.uid;

	if (!snapshot.description.empty())
	    s << " description:\"" << snapshot.description << "\"";

	if (!snapshot.cleanup.empty())
	    s << " cleanup:\"" << snapshot.cleanup << "\"";

	if (!snapshot.userdata.empty())
	    s << " userdata:\"" << snapshot.userdata << "\"";

	return s;
    }


    Snapshot::Snapshot(const Snapper* snapper, SnapshotType type, unsigned int num, time_t date)
	: snapper(snapper), type(type), num(num), date(date)
    {
    }


    Snapshot::~Snapshot()
    {
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


    SDir
    Snapshot::openInfoDir() const
    {
	if (isCurrent())
	    SN_THROW(IllegalSnapshotException());

	SDir infos_dir = snapper->openInfosDir();
	return SDir(infos_dir, decString(num));
    }


    SDir
    Snapshot::openSnapshotDir() const
    {
	if (isCurrent())
	    return snapper->openSubvolumeDir();

	return snapper->getFilesystem()->openSnapshotDir(num);
    }


    bool
    Snapshot::isReadOnly() const
    {
	if (isCurrent())
	    return false;

	return snapper->getFilesystem()->isSnapshotReadOnly(num);
    }


    bool
    Snapshot::isDefault() const
    {
	return snapper->getFilesystem()->isDefault(num);
    }


    void
    Snapshot::setDefault() const
    {
	return snapper->getFilesystem()->setDefault(num);
    }


    bool
    Snapshot::isActive() const
    {
	return !isCurrent() && snapper->getFilesystem()->isActive(num);
    }


    uint64_t
    Snapshot::getUsedSpace() const
    {
#ifdef ENABLE_BTRFS_QUOTA

	const Btrfs* btrfs = dynamic_cast<const Btrfs*>(snapper->getFilesystem());
	if (!btrfs)
	    SN_THROW(QuotaException("quota only supported with btrfs"));

	SDir general_dir = btrfs->openGeneralDir();

	subvolid_t subvolid = get_id(openSnapshotDir().fd());
	qgroup_t qgroup = calc_qgroup(0, subvolid);

	QGroupUsage qgroup_usage = qgroup_query_usage(general_dir.fd(), qgroup);

	return qgroup_usage.exclusive;

#else

	SN_THROW(QuotaException("not implemented"));
	__builtin_unreachable();

#endif
    }


    Snapshots::Snapshots(const Snapper* snapper)
	: snapper(snapper)
    {
    }


    Snapshots::~Snapshots()
    {
    }


    void
    Snapshots::read()
    {
	static const regex rx("[0-9]+", regex::extended);

	SDir infos_dir = snapper->openInfosDir();

	vector<string> infos = infos_dir.entries();
	for (vector<string>::const_iterator it1 = infos.begin(); it1 != infos.end(); ++it1)
	{
	    if (!regex_match(*it1, rx))
		continue;

	    try
	    {
		SDir info_dir(infos_dir, *it1);
		int fd = info_dir.open("info.xml", O_NOFOLLOW | O_CLOEXEC);
		if (fd < 0)
		    SN_THROW(IOErrorException("open info.xml failed"));

		XmlFile file(fd, "");

		const xmlNode* node = file.getRootElement();

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

		*it1 >> num;
		if (num != snapshot.num)
		{
		    y2err("num mismatch. not adding snapshot " << *it1);
		    continue;
		}

		getChildValue(node, "uid", snapshot.uid);

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
	    catch (const Exception& e)
	    {
		SN_CAUGHT(e);

		y2err("loading " << *it1 << " failed");
	    }
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
    Snapshots::checkUserdata(const map<string, string>& userdata) const
    {
	for (map<string, string>::const_iterator it = userdata.begin(); it != userdata.end(); ++it)
	{
	    if (it->first.empty() || it->first.find_first_of(",=") != string::npos)
		SN_THROW(InvalidUserdataException());

	    if (it->second.find_first_of(",=") != string::npos)
		SN_THROW(InvalidUserdataException());
	}
    }


    void
    Snapshots::initialize()
    {
	entries.clear();

	Snapshot snapshot(snapper, SINGLE, 0, (time_t)(-1));
	snapshot.description = "current";
	entries.push_back(snapshot);

	try
	{
	    read();
	}
	catch (const IOErrorException& e)
	{
	    SN_CAUGHT(e);

	    y2err("reading failed");
	}

	check();
    }


    Snapshots::iterator
    Snapshots::findPost(const_iterator pre)
    {
	if (pre == entries.end() || pre->isCurrent() || pre->getType() != PRE)
	    SN_THROW(IllegalSnapshotException());

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
	    SN_THROW(IllegalSnapshotException());

	for (const_iterator it = begin(); it != end(); ++it)
	{
	    if (it->getType() == POST && it->getPreNum() == pre->getNum())
		return it;
	}

	return end();
    }


    Snapshots::iterator
    Snapshots::findPre(const_iterator post)
    {
	if (post == entries.end() || post->isCurrent() || post->getType() != POST)
	    SN_THROW(IllegalSnapshotException());

	return find(post->pre_num);
    }


    Snapshots::const_iterator
    Snapshots::findPre(const_iterator post) const
    {
	if (post == entries.end() || post->isCurrent() || post->getType() != POST)
	    SN_THROW(IllegalSnapshotException());

	return find(post->pre_num);
    }


    unsigned int
    Snapshots::nextNumber()
    {
	unsigned int num = entries.empty() ? 0 : entries.rbegin()->num;

	SDir infos_dir = snapper->openInfosDir();

	while (true)
	{
	    ++num;

	    if (snapper->getFilesystem()->checkSnapshot(num))
		continue;

	    if (infos_dir.mkdir(decString(num), 0777) == 0)
		break;

	    if (errno == EEXIST)
		continue;

	    SN_THROW(IOErrorException(sformat("mkdir failed errno:%d (%s)", errno,
					      stringerror(errno).c_str())));
	}

	infos_dir.chmod(decString(num), 0755, 0);

	return num;
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

	if (uid != 0)
	    setChildValue(node, "uid", uid);

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

	string file_name = "info.xml";
	string tmp_name = file_name + ".tmp-XXXXXX";

	SDir info_dir = openInfoDir();

	int fd = info_dir.mktemp(tmp_name);
	if (fd < 0)
	    SN_THROW(IOErrorException("mktemp failed"));

	try
	{
	    xml.save(fd);
	}
	catch (const Exception& e)
	{
	    SN_CAUGHT(e);

	    info_dir.unlink(tmp_name, 0);

	    SN_RETHROW(e);
	}

	if (info_dir.rename(tmp_name, file_name) != 0)
	{
	    SN_THROW(IOErrorException(sformat("rename info.xml failed infoDir:%s errno:%d (%s)",
					      info_dir.fullname().c_str(), errno,
					      stringerror(errno).c_str())));
	}

	info_dir.fsync();
    }


    void
    Snapshot::mountFilesystemSnapshot(bool user_request) const
    {
	if (isCurrent())
	    SN_THROW(IllegalSnapshotException());

	if (!mount_checked)
	{
	    mount_user_request = snapper->getFilesystem()->isSnapshotMounted(num);
	    mount_checked = true;
	}

	if (user_request)
	    mount_user_request = true;
	else
	    mount_use_count++;

	snapper->getFilesystem()->mountSnapshot(num);
    }


    void
    Snapshot::umountFilesystemSnapshot(bool user_request) const
    {
	if (isCurrent())
	    SN_THROW(IllegalSnapshotException());

	if (!mount_checked)
	{
	    mount_user_request = snapper->getFilesystem()->isSnapshotMounted(num);
	    mount_checked = true;
	}

	if (user_request)
	    mount_user_request = false;
	else
	    mount_use_count--;

	if (user_request && mount_use_count == 0)
	    snapper->getFilesystem()->umountSnapshot(num);
    }


    void
    Snapshot::handleUmountFilesystemSnapshot() const
    {
	if (!mount_checked)
	    return;

	if (!mount_user_request && mount_use_count == 0)
	    snapper->getFilesystem()->umountSnapshot(num);
    }


    void
    Snapshot::createFilesystemSnapshot(unsigned int num_parent, bool read_only, bool empty) const
    {
	if (isCurrent())
	    SN_THROW(IllegalSnapshotException());

	snapper->getFilesystem()->createSnapshot(num, num_parent, read_only, !cleanup.empty(),
						 empty);
    }


    void
    Snapshot::createFilesystemSnapshotOfDefault(bool read_only) const
    {
	if (isCurrent())
	    SN_THROW(IllegalSnapshotException());

	snapper->getFilesystem()->createSnapshotOfDefault(num, read_only, !cleanup.empty());
    }


    void
    Snapshot::deleteFilesystemSnapshot() const
    {
	snapper->getFilesystem()->umountSnapshot(num);
	snapper->getFilesystem()->deleteSnapshot(num);
    }


    Snapshots::iterator
    Snapshots::createSingleSnapshot(const SCD& scd)
    {
	checkUserdata(scd.userdata);

	Snapshot snapshot(snapper, SINGLE, nextNumber(), time(NULL));
	snapshot.uid = scd.uid;
	snapshot.description = scd.description;
	snapshot.cleanup = scd.cleanup;
	snapshot.userdata = scd.userdata;

	return createHelper(snapshot, getSnapshotCurrent(), scd.read_only, scd.empty);
    }


    Snapshots::iterator
    Snapshots::createSingleSnapshot(const_iterator parent, const SCD& scd)
    {
	checkUserdata(scd.userdata);

	Snapshot snapshot(snapper, SINGLE, nextNumber(), time(NULL));
	snapshot.uid = scd.uid;
	snapshot.description = scd.description;
	snapshot.cleanup = scd.cleanup;
	snapshot.userdata = scd.userdata;

	return createHelper(snapshot, parent, scd.read_only);
    }


    Snapshots::iterator
    Snapshots::createSingleSnapshotOfDefault(const SCD& scd)
    {
	checkUserdata(scd.userdata);

	Snapshot snapshot(snapper, SINGLE, nextNumber(), time(NULL));
	snapshot.uid = scd.uid;
	snapshot.description = scd.description;
	snapshot.cleanup = scd.cleanup;
	snapshot.userdata = scd.userdata;

	return createHelper(snapshot, end(), scd.read_only);
    }


    Snapshots::iterator
    Snapshots::createPreSnapshot(const SCD& scd)
    {
	checkUserdata(scd.userdata);

	Snapshot snapshot(snapper, PRE, nextNumber(), time(NULL));
	snapshot.uid = scd.uid;
	snapshot.description = scd.description;
	snapshot.cleanup = scd.cleanup;
	snapshot.userdata = scd.userdata;

	return createHelper(snapshot, getSnapshotCurrent(), scd.read_only);
    }


    Snapshots::iterator
    Snapshots::createPostSnapshot(Snapshots::const_iterator pre, const SCD& scd)
    {
	if (pre == entries.end() || pre->isCurrent() || pre->getType() != PRE ||
	    findPost(pre) != entries.end())
	    SN_THROW(IllegalSnapshotException());

	checkUserdata(scd.userdata);

	Snapshot snapshot(snapper, POST, nextNumber(), time(NULL));
	snapshot.pre_num = pre->getNum();
	snapshot.uid = scd.uid;
	snapshot.description = scd.description;
	snapshot.cleanup = scd.cleanup;
	snapshot.userdata = scd.userdata;

	return createHelper(snapshot, getSnapshotCurrent(), scd.read_only);
    }


    Snapshots::iterator
    Snapshots::createHelper(Snapshot& snapshot, const_iterator parent, bool read_only,
			    bool empty)
    {
	// parent == end indicates the btrfs default subvolume. Unclean, but
	// adding a special snapshot like current needs too many API changes.

	try
	{
	    if (parent != end())
		snapshot.createFilesystemSnapshot(parent->getNum(), read_only, empty);
	    else
		snapshot.createFilesystemSnapshotOfDefault(read_only);
	}
	catch (const CreateSnapshotFailedException& e)
	{
	    SN_CAUGHT(e);

	    SDir infos_dir = snapper->openInfosDir();
	    infos_dir.unlink(decString(snapshot.getNum()), AT_REMOVEDIR);

	    SN_RETHROW(e);
	}

	try
	{
	    snapshot.writeInfo();
	}
	catch (const IOErrorException& e)
	{
	    SN_CAUGHT(e);

	    snapshot.deleteFilesystemSnapshot();
	    SDir infos_dir = snapper->openInfosDir();
	    infos_dir.unlink(decString(snapshot.getNum()), AT_REMOVEDIR);

	    SN_RETHROW(e);
	}

	Hooks::create_snapshot(snapper->subvolumeDir(), snapper->getFilesystem());

	return entries.insert(entries.end(), snapshot);
    }


    void
    Snapshots::modifySnapshot(iterator snapshot, const SMD& smd)
    {
	if (snapshot == entries.end() || snapshot->isCurrent())
	    SN_THROW(IllegalSnapshotException());

	checkUserdata(smd.userdata);

	snapshot->description = smd.description;
	snapshot->cleanup = smd.cleanup;
	snapshot->userdata = smd.userdata;

	snapshot->writeInfo();

	Hooks::modify_snapshot(snapper->subvolumeDir(), snapper->getFilesystem());
    }


    void
    Snapshots::deleteSnapshot(iterator snapshot)
    {
	if (snapshot == entries.end() || snapshot->isCurrent() || snapshot->isDefault() ||
	    snapshot->isActive())
	    SN_THROW(IllegalSnapshotException());

	snapshot->deleteFilesystemSnapshot();

	SDir info_dir = snapshot->openInfoDir();

	info_dir.unlink("info.xml", 0);

	// remove all filelists in the info directory of this shapshot
	vector<string> tmp1 = info_dir.entries(is_filelist_file);
	for (const string& name : tmp1)
	{
	    info_dir.unlink(name, 0);
	}

	// remove all filelists of the snapshot in the info directories of other snapshots
	for (Snapshots::iterator it = begin(); it != end(); ++it)
	{
	    if (!it->isCurrent())
	    {
		SDir tmp2 = it->openInfoDir();
		string name = filelist_name(snapshot->getNum());
		tmp2.unlink(name, 0);
		tmp2.unlink(name + ".gz", 0);
	    }
	}

	SDir infos_dir = snapper->openInfosDir();
	infos_dir.unlink(decString(snapshot->getNum()), AT_REMOVEDIR);

	entries.erase(snapshot);

	Hooks::delete_snapshot(snapper->subvolumeDir(), snapper->getFilesystem());
    }


    Snapshots::iterator
    Snapshots::getDefault()
    {
	std::pair<bool, unsigned int> tmp = snapper->getFilesystem()->getDefault();

	return tmp.first ? find(tmp.second) : end();
    }


    Snapshots::const_iterator
    Snapshots::getDefault() const
    {
	std::pair<bool, unsigned int> tmp = snapper->getFilesystem()->getDefault();

	return tmp.first ? find(tmp.second) : end();
    }


    Snapshots::const_iterator
    Snapshots::getActive() const
    {
	std::pair<bool, unsigned int> tmp = snapper->getFilesystem()->getActive();

	return tmp.first ? find(tmp.second) : end();
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
