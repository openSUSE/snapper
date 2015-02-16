/*
 * Copyright (c) 2015 Novell, Inc.
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

#include "utils/GetOpts.h"


using namespace snapper;
using namespace std;


void
step1(const string& device)
{
    // step runs in inst-sys

    cout << "step 1 device:" << device << endl;

    cout << "temporarily mounting device" << endl;

    SDir s_dir("/");

    TmpMount tmp_mount(s_dir, device, "tmp-mnt-XXXXXX", "btrfs", 0, "");

    cout << "copying config-file" << endl;

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
	cerr << "copying config-file failed" << endl;
    }

    cout << "creating filesystem config" << endl;

    Btrfs btrfs("/", tmp_mount.getFullname());

    btrfs.createConfig();

    cout << "creating snapshot" << endl;

    Snapper snapper("root", tmp_mount.getFullname());

    SCD scd;
    scd.read_only = false;

    Snapshots::iterator snapshot = snapper.createSingleSnapshot(scd);

    cout << "setting default subvolume" << endl;

    snapper.getFilesystem()->setDefault(snapshot->getNum());

    cout << "done" << endl;
}


void
step2(const string& device, const string& root_prefix, const string& default_subvolume_name)
{
    // step runs in inst-sys

    cout << "step 2 device:" << device << " root-prefix:" << root_prefix
	 << " default-subvolume-name:" << default_subvolume_name << endl;

    cout << "mounting device" << endl;

    string subvol_option = default_subvolume_name;
    if (!subvol_option.empty())
	subvol_option += "/";
    subvol_option += ".snapshots";

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


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    const struct option options[] = {
	{ "step",			required_argument,	0,	0 },
	{ "device",			required_argument,	0,	0 },
	{ "root-prefix",		required_argument,	0,	0 },
	{ "default-subvolume-name",	required_argument,	0,	0 },
	{ 0, 0, 0, 0 }
    };

    string step;
    string device;
    string root_prefix = "/";
    string default_subvolume_name;

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

    if (step == "1")
	step1(device);
    else if (step == "2")
	step2(device, root_prefix, default_subvolume_name);
    else if (step == "3")
	step3(root_prefix, default_subvolume_name);
    else if (step == "4")
	step4();
}
