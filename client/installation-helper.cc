/*
 * Copyright (c) 2015 Novell, Inc.
 * Copyright (c) 2018 SUSE LLC
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

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <boost/algorithm/string.hpp>

#include <snapper/Snapper.h>
#include <snapper/AppUtil.h>
#include <snapper/SnapperDefines.h>
#include <snapper/Btrfs.h>
#include <snapper/FileUtils.h>
#include <snapper/Hooks.h>
#include "snapper/Log.h"

#include "utils/GetOpts.h"

#include "misc.h"


using namespace snapper;
using namespace std;


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
	SysconfigFile config(CONFIGTEMPLATEDIR "/" "default");

	config.setName(tmp_mount.getFullname() + CONFIGSDIR "/" "root");

	config.setValue(KEY_SUBVOLUME, "/");
	config.setValue(KEY_FSTYPE, "btrfs");
    }
    catch (const FileNotFoundException& e)
    {
	cerr << "copying/modifying config-file failed" << endl;
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

    Snapshots::iterator snapshot = snapper.createSingleSnapshot(scd);

    cout << "again copying config-file" << endl;

    string ris = tmp_mount.getFullname() + snapshot->snapshotDir();

    mkdir((ris + "/etc").c_str(), 0777);
    mkdir((ris + "/etc/snapper").c_str(), 0777);
    mkdir((ris + "/etc/snapper/configs").c_str(), 0777);

    system(("/bin/cp " + tmp_mount.getFullname() + "/etc/snapper/configs/root " + ris +
	    "/etc/snapper/configs").c_str());

    cout << "setting default subvolume" << endl;

    snapper.getFilesystem()->setDefault(snapshot->getNum());

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
    subvol_option += ".snapshots";

    mkdir((root_prefix + "/.snapshots").c_str(), 0777);

    SDir s_dir(root_prefix + "/.snapshots");
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
    // all programs for snapper hooks installed in chroot
    // all preconditions for hooks satisfied

    cout << "step 4" << endl;

    cout << "modifying sysconfig-file" << endl;

    try
    {
	SysconfigFile sysconfig(SYSCONFIGFILE);
	sysconfig.setValue("SNAPPER_CONFIGS", { "root" });
    }
    catch (const FileNotFoundException& e)
    {
	cerr << "sysconfig-file not found" << endl;
    }

    Btrfs btrfs("/", "");

    cout << "running external programs" << endl;

    Hooks::create_config("/", &btrfs);

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
            snapshot = snapper.createSingleSnapshot(scd);
        } else if (snapshot_type == "pre") {
            snapshot = snapper.createPreSnapshot(scd);
        } else if (snapshot_type == "post") {
            Snapshots snapshots = snapper.getSnapshots();
            Snapshots::iterator pre = snapshots.find(pre_num);
            snapshot = snapper.createPostSnapshot(pre, scd);
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


void
log_do(LogLevel level, const string& component, const char* file, const int line, const char* func,
       const string& text)
{
    cerr << text << endl;
}


bool
log_query(LogLevel level, const string& component)
{
    return level == ERROR;
}


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    setLogDo(&log_do);
    setLogQuery(&log_query);

    const struct option options[] = {
	{ "step",			required_argument,	0,	0 },
	{ "device",			required_argument,	0,	0 },
	{ "root-prefix",		required_argument,	0,	0 },
	{ "default-subvolume-name",	required_argument,	0,	0 },
	{ "snapshot-type",		required_argument,	0,	0 },
	{ "pre-num",     		required_argument,	0,	0 },
	{ "description",		required_argument,	0,	0 },
	{ "cleanup",			required_argument,	0,	0 },
	{ "userdata",			required_argument,	0,	0 },
	{ 0, 0, 0, 0 }
    };

    string step;
    string device;
    string root_prefix = "/";
    string default_subvolume_name;
    string snapshot_type = "single";
    unsigned int pre_num = 0;
    string description;
    string cleanup;
    map<string, string> userdata;

    GetOpts getopts;

    getopts.init(argc, argv);

    GetOpts::parsed_opts opts = getopts.parse(options);

    GetOpts::parsed_opts::const_iterator opt;

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
}
