/*
 * Copyright (c) [2011-2013] Novell, Inc.
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
#include <glob.h>
#include <string.h>
#include <mntent.h>
#include <boost/algorithm/string.hpp>

#include "snapper/Snapper.h"
#include "snapper/Comparison.h"
#include "snapper/AppUtil.h"
#include "snapper/Enum.h"
#include "snapper/Filesystem.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/File.h"
#include "snapper/AsciiFile.h"
#include "snapper/Exception.h"


namespace snapper
{
    using namespace std;


    ConfigInfo::ConfigInfo(const string& config_name)
	: SysconfigFile(CONFIGSDIR "/" + config_name), config_name(config_name), subvolume("/")
    {
	if (!getValue("SUBVOLUME", subvolume))
	    throw InvalidConfigException();
    }


    void
    ConfigInfo::checkKey(const string& key) const
    {
	if (key == "SUBVOLUME" || key == "FSTYPE")
	    throw InvalidConfigdataException();

	try
	{
	    SysconfigFile::checkKey(key);
	}
	catch (const InvalidKeyException& e)
	{
	    throw InvalidConfigdataException();
	}
    }


    Snapper::Snapper(const string& config_name, bool disable_filters)
	: config_info(NULL), filesystem(NULL), snapshots(this)
    {
	y2mil("Snapper constructor");
	y2mil("libsnapper version " VERSION);
	y2mil("config_name:" << config_name << " disable_filters:" << disable_filters);

	try
	{
	    config_info = new ConfigInfo(config_name);
	}
	catch (const FileNotFoundException& e)
	{
	    throw ConfigNotFoundException();
	}

	string fstype = "btrfs";
	config_info->getValue("FSTYPE", fstype);
	filesystem = Filesystem::create(fstype, config_info->getSubvolume());

	y2mil("subvolume:" << config_info->getSubvolume() << " filesystem:" <<
	      filesystem->fstype());

	if (!disable_filters)
	    loadIgnorePatterns();

	snapshots.initialize();
    }


    Snapper::~Snapper()
    {
	y2mil("Snapper destructor");

	for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	{
	    it->flushInfo();

	    try
	    {
		it->handleUmountFilesystemSnapshot();
	    }
	    catch (const UmountSnapshotFailedException& e)
	    {
	    }
	}

	delete filesystem;
	delete config_info;
    }


    void
    Snapper::loadIgnorePatterns()
    {
	const list<string> files = glob(FILTERSDIR "/*.txt", GLOB_NOSORT);
	for (list<string>::const_iterator it = files.begin(); it != files.end(); ++it)
	{
	    try
	    {
		AsciiFileReader asciifile(*it);

		string line;
		while (asciifile.getline(line))
		    if (!line.empty())
			ignore_patterns.push_back(line);
	    }
	    catch (const FileNotFoundException& e)
	    {
	    }
	}

	y2mil("number of ignore patterns:" << ignore_patterns.size());
    }


    // Directory of which snapshots are made, e.g. "/" or "/home".
    string
    Snapper::subvolumeDir() const
    {
	return config_info->getSubvolume();
    }


    SDir
    Snapper::openSubvolumeDir() const
    {
	return filesystem->openSubvolumeDir();
    }


    SDir
    Snapper::openInfosDir() const
    {
	return filesystem->openInfosDir();
    }


    Snapshots::const_iterator
    Snapper::getSnapshotCurrent() const
    {
	return snapshots.getSnapshotCurrent();
    }


    Snapshots::iterator
    Snapper::createSingleSnapshot(string description)
    {
	return snapshots.createSingleSnapshot(description);
    }


    Snapshots::iterator
    Snapper::createPreSnapshot(string description)
    {
	return snapshots.createPreSnapshot(description);
    }


    Snapshots::iterator
    Snapper::createPostSnapshot(string description, Snapshots::const_iterator pre)
    {
	return snapshots.createPostSnapshot(description, pre);
    }


    void
    Snapper::deleteSnapshot(Snapshots::iterator snapshot)
    {
	snapshots.deleteSnapshot(snapshot);
    }


    ConfigInfo
    Snapper::getConfig(const string& config_name)
    {
	return ConfigInfo(config_name);
    }


    list<ConfigInfo>
    Snapper::getConfigs()
    {
	y2mil("Snapper get-configs");
	y2mil("libsnapper version " VERSION);

	list<ConfigInfo> config_infos;

	try
	{
	    SysconfigFile sysconfig(SYSCONFIGFILE);
	    vector<string> config_names;
	    sysconfig.getValue("SNAPPER_CONFIGS", config_names);

	    for (vector<string>::const_iterator it = config_names.begin(); it != config_names.end(); ++it)
	    {
		try
		{
		    config_infos.push_back(getConfig(*it));
		}
		catch (const FileNotFoundException& e)
		{
		    y2err("config '" << *it << "' not found");
		}
		catch (const InvalidConfigException& e)
		{
		    y2err("config '" << *it << "' is invalid");
		}
	    }
	}
	catch (const FileNotFoundException& e)
	{
	    throw ListConfigsFailedException("sysconfig-file not found");
	}

	return config_infos;
    }


    void
    Snapper::createConfig(const string& config_name, const string& subvolume,
			  const string& fstype, const string& template_name)
    {
	y2mil("Snapper create-config");
	y2mil("libsnapper version " VERSION);
	y2mil("config_name:" << config_name << " subvolume:" << subvolume <<
	      " fstype:" << fstype << " template_name:" << template_name);

	if (config_name.empty() || config_name.find_first_of(", \t") != string::npos)
	{
	    throw CreateConfigFailedException("illegal config name");
	}

	if (!boost::starts_with(subvolume, "/") || !checkDir(subvolume))
	{
	    throw CreateConfigFailedException("illegal subvolume");
	}

	list<ConfigInfo> configs = getConfigs();
	for (list<ConfigInfo>::const_iterator it = configs.begin(); it != configs.end(); ++it)
	{
	    if (it->getSubvolume() == subvolume)
	    {
		throw CreateConfigFailedException("subvolume already covered");
	    }
	}

	if (access(string(CONFIGTEMPLATEDIR "/" + template_name).c_str(), R_OK) != 0)
	{
	    throw CreateConfigFailedException("cannot access template config");
	}

	auto_ptr<Filesystem> filesystem;
	try
	{
	    filesystem.reset(Filesystem::create(fstype, subvolume));
	}
	catch (const InvalidConfigException& e)
	{
	    throw CreateConfigFailedException("invalid filesystem type");
	}
	catch (const ProgramNotInstalledException& e)
	{
	    throw CreateConfigFailedException(e.what());
	}

	try
	{
	    SysconfigFile sysconfig(SYSCONFIGFILE);
	    vector<string> config_names;
	    sysconfig.getValue("SNAPPER_CONFIGS", config_names);
	    if (find(config_names.begin(), config_names.end(), config_name) != config_names.end())
	    {
		throw CreateConfigFailedException("config already exists");
	    }

	    config_names.push_back(config_name);
	    sysconfig.setValue("SNAPPER_CONFIGS", config_names);
	}
	catch (const FileNotFoundException& e)
	{
	    throw CreateConfigFailedException("sysconfig-file not found");
	}

	SystemCmd cmd1(CPBIN " " + quote(CONFIGTEMPLATEDIR "/" + template_name) + " " +
		       quote(CONFIGSDIR "/" + config_name));
	if (cmd1.retcode() != 0)
	{
	    throw CreateConfigFailedException("copying config-file template failed");
	}

	try
	{
	    SysconfigFile config(CONFIGSDIR "/" + config_name);
	    config.setValue("SUBVOLUME", subvolume);
	    config.setValue("FSTYPE", filesystem->fstype());
	}
	catch (const FileNotFoundException& e)
	{
	    throw CreateConfigFailedException("modifying config failed");
	}

	filesystem->createConfig();
    }


    void
    Snapper::deleteConfig(const string& config_name)
    {
	y2mil("Snapper delete-config");
	y2mil("libsnapper version " VERSION);

	auto_ptr<Snapper> snapper(new Snapper(config_name));

	Snapshots& snapshots = snapper->getSnapshots();
	for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); )
	{
	    Snapshots::iterator tmp = it++;

	    if (tmp->isCurrent())
		continue;

	    try
	    {
		snapper->deleteSnapshot(tmp);
	    }
	    catch (const DeleteSnapshotFailedException& e)
	    {
		// ignore, Filesystem->deleteConfig will fail anyway
	    }
	}

	try
	{
	    snapper->getFilesystem()->deleteConfig();
	}
	catch (const DeleteConfigFailedException& e)
	{
	    throw DeleteConfigFailedException("deleting snapshot failed");
	}

	SystemCmd cmd1(RMBIN " " + quote(CONFIGSDIR "/" + config_name));
	if (cmd1.retcode() != 0)
	{
	    throw DeleteConfigFailedException("deleting config-file failed");
	}

	try
	{
	    SysconfigFile sysconfig(SYSCONFIGFILE);
	    vector<string> config_names;
	    sysconfig.getValue("SNAPPER_CONFIGS", config_names);
	    config_names.erase(remove(config_names.begin(), config_names.end(), config_name),
			       config_names.end());
	    sysconfig.setValue("SNAPPER_CONFIGS", config_names);
	}
	catch (const FileNotFoundException& e)
	{
	    throw DeleteConfigFailedException("sysconfig-file not found");
	}
    }


    static bool
    is_subpath(const string& a, const string& b)
    {
	if (b == "/")
	    return true;

	size_t len = b.length();

	if (len > a.length())
	    return false;

	return (len == a.length() || a[len] == '/') && a.compare(0, len, b) == 0;
    }


    bool
    Snapper::detectFstype(const string& subvolume, string& fstype)
    {
	y2mil("subvolume:" << subvolume);

	if (!boost::starts_with(subvolume, "/") || !checkDir(subvolume))
	    return false;

	FILE* f = setmntent("/etc/mtab", "r");
	if (!f)
	{
	    y2err("setmntent failed");
	    return false;
	}

	fstype.clear();

	string best_match;

	struct mntent* m;
	while ((m = getmntent(f)))
	{
	    if (strcmp(m->mnt_type, "rootfs") == 0)
		continue;

	    if (strlen(m->mnt_dir) >= best_match.length() && is_subpath(subvolume, m->mnt_dir))
	    {
		best_match = m->mnt_dir;
		fstype = m->mnt_type;
	    }
	}

	endmntent(f);

	if (fstype == "ext4dev")
	    fstype = "ext4";

	y2mil("fstype:" << fstype);

	return !best_match.empty();
    }

}
