/*
 * Copyright (c) 2015 Novell, Inc.
 * Copyright (c) [2018-2025] SUSE LLC
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

#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>

#include <snapper/Snapper.h>
#include <snapper/AppUtil.h>
#include <snapper/SnapperDefines.h>
#include <snapper/Btrfs.h>
#include <snapper/FileUtils.h>
#include <snapper/PluginsImpl.h>
#include "snapper/Logger.h"
#include "snapper/XmlFile.h"

#include "../utils/GetOpts.h"

#include "../misc.h"


using namespace snapper;
using namespace std;


Plugins::Report report;


void
step1(const string& device, const string& description, const string& cleanup,
      const map<string, string>& userdata)
{
    // step runs in inst-sys

    // preconditions (maybe incomplete):
    // device is formatted with btrfs
    // default subvolume for device is set to (for installation) final value

    cout << "step 1 device:" << device << endl;

    cout << "temporarily mounting device" << endl;

    SDir s_dir("/");

    TmpMount tmp_mount(s_dir, device, "tmp-mnt-XXXXXX", "btrfs", 0, "");

    cout << "copying/modifying config-file" << endl;

    mkdir((tmp_mount.getFullname() + "/etc").c_str(), 0777);
    mkdir((tmp_mount.getFullname() + "/etc/snapper").c_str(), 0777);
    mkdir((tmp_mount.getFullname() + "/etc/snapper/configs").c_str(), 0777);

    try
    {
	SysconfigFile config(locate_file("default", ETC_CONFIG_TEMPLATE_DIR, USR_CONFIG_TEMPLATE_DIR));

	config.set_name(tmp_mount.getFullname() + CONFIGS_DIR "/" "root");

	config.set_value(KEY_SUBVOLUME, "/");
	config.set_value(KEY_FSTYPE, "btrfs");

	config.save();
    }
    catch (const Exception& e)
    {
	cerr << "copying/modifying config-file failed"
	     << e.what() << endl;
    }

    cout << "creating filesystem config" << endl;

    Btrfs btrfs("/", tmp_mount.getFullname());

    btrfs.createConfig();

    cout << "creating subvolume" << endl;

    Snapper snapper("root", tmp_mount.getFullname());

    SCD scd;
    scd.read_only = false;
    scd.empty = true;
    scd.description = description;
    scd.cleanup = cleanup;
    scd.userdata = userdata;

    Snapshots::iterator snapshot = snapper.createSingleSnapshot(scd, report);

    cout << "again copying config-file" << endl;

    string ris = tmp_mount.getFullname() + snapshot->snapshotDir();

    mkdir((ris + "/etc").c_str(), 0777);
    mkdir((ris + "/etc/snapper").c_str(), 0777);
    mkdir((ris + "/etc/snapper/configs").c_str(), 0777);

    system(("/bin/cp " + tmp_mount.getFullname() + "/etc/snapper/configs/root " + ris +
	    "/etc/snapper/configs").c_str());

    cout << "setting default subvolume" << endl;

    snapshot->setDefault(report);

    cout << "done" << endl;
}


void
step2(const string& device, const string& root_prefix, const string& default_subvolume_name)
{
    // step runs in inst-sys

    // preconditions (maybe incomplete):
    // default subvolume of device is mounted at root-prefix
    // .snapshots subvolume is not mounted

    cout << "step 2 device:" << device << " root-prefix:" << root_prefix
	 << " default-subvolume-name:" << default_subvolume_name << endl;

    cout << "mounting device" << endl;

    // The btrfs subvol mount option is either "@/.snapshots" or just
    // ".snapshots" depending on whether default_subvolume_name is e.g. "@" or
    // "".
    string subvol_option = default_subvolume_name;
    if (!subvol_option.empty())
	subvol_option += "/";
    subvol_option += SNAPSHOTS_NAME;

    mkdir((root_prefix + "/" SNAPSHOTS_NAME).c_str(), 0777);

    SDir s_dir(root_prefix + "/" SNAPSHOTS_NAME);
    if (!s_dir.mount(device, "btrfs", 0, "subvol=" + subvol_option))
    {
	cerr << "mounting .snapshots failed" << endl;
    }

    cout << "done" << endl;
}


void
step3(const string& root_prefix, const string& default_subvolume_name)
{
    // step runs in inst-sys

    // preconditions (maybe incomplete):
    // default subvolume of device is mounted at root-prefix
    // fstab at root-prefix contains entry for default subvolume of device

    cout << "step 3 root-prefix:" << root_prefix << " default_subvolume_name:"
	 << default_subvolume_name << endl;

    cout << "adding .snapshots to fstab" << endl;

    Btrfs btrfs("/", root_prefix);

    btrfs.addToFstab(default_subvolume_name);

    cout << "done" << endl;
}


void
step4()
{
    // step runs in chroot

    // preconditions (maybe incomplete):
    // snapper rpms installed in chroot
    // all programs for snapper plugins installed in chroot
    // all preconditions for plugins satisfied

    cout << "step 4" << endl;

    cout << "modifying sysconfig-file" << endl;

    try
    {
	SysconfigFile sysconfig(SYSCONFIG_FILE);

	sysconfig.set_value("SNAPPER_CONFIGS", { "root" });

	sysconfig.save();
    }
    catch (const Exception& e)
    {
	cerr << "modifying sysconfig-file failed"
	     << e.what() << endl;
    }

    Btrfs btrfs("/", "");

    cout << "running external programs" << endl;

    Plugins::create_config(Plugins::Stage::POST_ACTION, "/", &btrfs, report);

    cout << "done" << endl;
}


bool
step5(const string& root_prefix, const string& snapshot_type, unsigned int pre_num,
      const string& description, const string& cleanup, const map<string, string>& userdata)
{
    // fate #317973

    // preconditions (maybe incomplete):
    // snapper rpms installed

    SCD scd;
    scd.read_only = true;
    scd.description = description;
    scd.cleanup = cleanup;
    scd.userdata = userdata;

    Snapper snapper("root", root_prefix);

    Snapshots::iterator snapshot;

    try
    {
        if (snapshot_type == "single") {
            snapshot = snapper.createSingleSnapshot(scd, report);
        } else if (snapshot_type == "pre") {
            snapshot = snapper.createPreSnapshot(scd, report);
        } else if (snapshot_type == "post") {
            Snapshots snapshots = snapper.getSnapshots();
            Snapshots::iterator pre = snapshots.find(pre_num);
            snapshot = snapper.createPostSnapshot(pre, scd, report);
        }
    }
    catch (const runtime_error& e)
    {
        y2err("create snapshot failed, " << e.what());
        return false;
    }

    cout << snapshot->getNum() << endl;
    return true;
}


// Without the rollback support of libsnapper some btrfs utils are undefined.
#ifdef ENABLE_ROLLBACK


bool
step_filesystem(const string& root_prefix)
{
    // create subvolume /<root-prefix>/.snapshots

    cout << "creating btrfs subvolume /<root-prefix>/.snapshots" << endl;

    try
    {
	int fd = open(prepend_root_prefix(root_prefix, "/").c_str(),
		      O_RDONLY | O_NOATIME | O_CLOEXEC);
	if (fd < 0)
	    SN_THROW(Exception("open failed"));

	create_subvolume(fd, ".snapshots");

	close(fd);
    }
    catch (const exception& e)
    {
	cerr << "creating /<root-prefix>/.snapshots failed" << endl;

	return false;
    }

    // create /<root-prefix>/.snapshots/1

    cout << "creating directory /<root-prefix>/.snapshots/1" << endl;

    try
    {
	if (mkdir(prepend_root_prefix(root_prefix, "/.snapshots/1").c_str(), 0777) != 0)
	    SN_THROW(Exception("mkdir failed"));
    }
    catch (const exception& e)
    {
	cerr << "creating /<root-prefix>/.snapshots/1 failed" << endl;

	return false;
    }

    // create btrfs subvolume /<root-prefix>/.snapshots/1/snapshot

    cout << "creating btrfs subvolume /<root-prefix>/.snapshots/1/snapshot" << endl;

    try
    {
	int fd = open(prepend_root_prefix(root_prefix, "/.snapshots/1").c_str(),
		      O_RDONLY | O_NOATIME | O_CLOEXEC);
	if (fd < 0)
	    SN_THROW(Exception("open failed"));

	create_subvolume(fd, "snapshot");

	close(fd);
    }
    catch (const exception& e)
    {
	cerr << "creating /<root-prefix>/.snapshots/1/snapshot failed" << endl;

	return false;
    }

    // set default btrfs subvolume to /<root-prefix>/.snapshots/1/snapshot

    cout << "setting default btrfs subvolume to /<root-prefix>/.snapshots/1/snapshot" << endl;

    try
    {
	int fd = open(prepend_root_prefix(root_prefix, "/.snapshots/1/snapshot").c_str(),
		      O_RDONLY | O_NOATIME | O_CLOEXEC);
	if (fd < 0)
	    SN_THROW(Exception("open failed"));

	subvolid_t id = get_id(fd);

	set_default_id(fd, id);

	close(fd);
    }
    catch (const exception& e)
    {
	cerr << "setting default subvolume failed" << endl;

	return false;
    }

    // create /<root-prefix>/.snapshots/1/snapshot/.snapshots

    cout << "creating directory /<root-prefix>/.snapshots/1/snapshot/.snapshots" << endl;

    try
    {
	if (mkdir(prepend_root_prefix(root_prefix, "/.snapshots/1/snapshot/.snapshots").c_str(), 0777) != 0)
	    SN_THROW(Exception("mkdir failed"));
    }
    catch (const exception& e)
    {
	cerr << "creating /<root-prefix>/.snapshots/1/snapshot/.snapshots failed" << endl;

	return false;
    }

    return true;
}


bool
step_config(const string& root_prefix, const string& description, const string& cleanup,
	    const map<string, string>& userdata)
{
    // create directories

    cout << "creating directories in /<root-prefix>" << endl;

    mkdir(prepend_root_prefix(root_prefix, "/etc").c_str(), 0777);
    mkdir(prepend_root_prefix(root_prefix, "/etc/sysconfig").c_str(), 0777);
    mkdir(prepend_root_prefix(root_prefix, "/etc/snapper").c_str(), 0777);
    mkdir(prepend_root_prefix(root_prefix, "/etc/snapper/configs").c_str(), 0777);

    // create snapper sysconfig /<root-prefix>/etc/sysconfig/snapper

    cout << "creating snapper sysconfig /<root-prefix>/etc/sysconfig/snapper" << endl;

    try
    {
	/*
	SysconfigFile sysconfig;	// TODO, add in class SysconfigFile

	sysconfig.set_name(prepend_root_prefix(root_prefix, SYSCONFIG_FILE));

	sysconfig.set_value("SNAPPER_CONFIGS", "root");

	sysconfig.save();
	*/

	AsciiFileWriter sysconfig(prepend_root_prefix(root_prefix, SYSCONFIG_FILE), Compression::NONE);

	sysconfig.write_line("SNAPPER_CONFIGS=\"root\"");

	sysconfig.close();
    }
    catch (const Exception& e)
    {
	cerr << "setup of /<root-prefix>/etc/sysconfig/snapper failed, "
	     << e.what() << endl;

	return false;
    }

    // create snapper config /<root-prefix>/etc/snapper/configs/root

    cout << "creating snapper config /<root-prefix>/etc/snapper/configs/root" << endl;

    try
    {
	string template_file = locate_file("default", ETC_CONFIG_TEMPLATE_DIR, USR_CONFIG_TEMPLATE_DIR);

	SysconfigFile config(template_file);

	config.set_name(prepend_root_prefix(root_prefix, CONFIGS_DIR "/root"));

	config.set_value(KEY_SUBVOLUME, "/");
	config.set_value(KEY_FSTYPE, "btrfs");

	config.save();
    }
    catch (const Exception& e)
    {
	cerr << "setup of /<root-prefix>/etc/snapper/configs/root failed, "
	     << e.what() << endl;

	return false;
    }

    // create info file /<root-prefix>/.snapshots/1/info.xml

    cout << "creating snapper info file /<root-prefix>/.snapshots/1/info.xml" << endl;

    try
    {
	XmlFile xml;
	xmlNode* node = xmlNewNode("snapshot");
	xml.setRootElement(node);

	setChildValue(node, "type", toString(SINGLE));

	setChildValue(node, "num", 1);

	setChildValue(node, "date", datetime(time(NULL), true, true));

	if (!description.empty())
	    setChildValue(node, "description", description);

	if (!cleanup.empty())
	    setChildValue(node, "cleanup", cleanup);

	for (const map<string, string>::value_type& tmp : userdata)
	{
	    xmlNode* userdata_node = xmlNewChild(node, "userdata");
	    setChildValue(userdata_node, "key", tmp.first);
	    setChildValue(userdata_node, "value", tmp.second);
	}

	int fd = open(prepend_root_prefix(root_prefix, "/.snapshots/1/info.xml").c_str(),
		      O_RDWR | O_CREAT | O_CLOEXEC, 0666);
	if (fd < 0)
	    SN_THROW(Exception("open failed"));

	fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	xml.save(fd);
    }
    catch (const Exception& e)
    {
	SN_CAUGHT(e);

	cerr << "setup of /<root-prefix>/.snapshots/1/info.xml failed, "
	     << e.what() << endl;

	return false;
    }

    return true;
}

#endif


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    set_logger(get_stdout_logger());

    string step;
    string device;
    string root_prefix = "/";
    string default_subvolume_name;
    string snapshot_type = "single";
    unsigned int pre_num = 0;
    string description;
    string cleanup;
    map<string, string> userdata;

    try
    {
	const vector<Option> options = {
	    Option("step",			required_argument),
	    Option("device",			required_argument),
	    Option("root-prefix",		required_argument),
	    Option("default-subvolume-name",	required_argument),
	    Option("snapshot-type",		required_argument),
	    Option("pre-num",			required_argument),
	    Option("description",		required_argument),
	    Option("cleanup",			required_argument),
	    Option("userdata",			required_argument)
	};

	GetOpts get_opts(argc, argv);

	ParsedOpts opts = get_opts.parse(options);

	ParsedOpts::const_iterator opt;

	if ((opt = opts.find("step")) != opts.end())
	    step = opt->second;

	if ((opt = opts.find("device")) != opts.end())
	    device = opt->second;

	if ((opt = opts.find("root-prefix")) != opts.end())
	    root_prefix = opt->second;

	if ((opt = opts.find("default-subvolume-name")) != opts.end())
	    default_subvolume_name = opt->second;

	if ((opt = opts.find("snapshot-type")) != opts.end())
	    snapshot_type = opt->second;

	if ((opt = opts.find("pre-num")) != opts.end())
	    pre_num = read_num(opt->second);

	if ((opt = opts.find("description")) != opts.end())
	    description = opt->second;

	if ((opt = opts.find("cleanup")) != opts.end())
	    cleanup = opt->second;

	if ((opt = opts.find("userdata")) != opts.end())
	    userdata = read_userdata(opt->second);
    }
    catch (const OptionsException& e)
    {
	SN_CAUGHT(e);
	cerr << e.what() << endl;
	return EXIT_FAILURE;
    }

    if (step == "1")
	step1(device, description, cleanup, userdata);
    else if (step == "2")
	step2(device, root_prefix, default_subvolume_name);
    else if (step == "3")
	step3(root_prefix, default_subvolume_name);
    else if (step == "4")
	step4();
    else if (step == "5")
	return step5(root_prefix, snapshot_type, pre_num, description, cleanup, userdata) ? EXIT_SUCCESS : EXIT_FAILURE;

#ifdef ENABLE_ROLLBACK

    else if (step == "filesystem")
	return step_filesystem(root_prefix) ? EXIT_SUCCESS : EXIT_FAILURE;
    else if (step == "config")
	return step_config(root_prefix, description, cleanup, userdata) ? EXIT_SUCCESS : EXIT_FAILURE;

#endif

}
