/*
 * Copyright (c) [2015-2017] SUSE LLC
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


#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ext2fs/ext2_fs.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <iostream>
#include <boost/algorithm/string.hpp>

#include "snapper/BtrfsUtils.h"
#include "snapper/AppUtil.h"
#include "snapper/MntTable.h"
#include "utils/GetOpts.h"


using namespace snapper;
using namespace BtrfsUtils;


string target;

bool force = false;
bool set_nocow = false;
bool verbose = false;


void
do_tmp_mount(const libmnt_fs* fs, const char* tmp_mountpoint, const string& subvol_option)
{
    if (verbose)
	cout << "do-tmp-mount" << endl;

    libmnt_fs* x = mnt_copy_fs(NULL, fs);
    if (!x)
	throw runtime_error("mnt_copy_fs failed");

    struct libmnt_context* cxt = mnt_new_context();
    mnt_context_set_fs(cxt, x);
    mnt_context_set_target(cxt, tmp_mountpoint);
    if (!subvol_option.empty())
	mnt_context_set_options(cxt, ("subvol=" + subvol_option).c_str());
    else
	mnt_context_set_options(cxt, "subvolid=5"); // 5 is the btrfs top-level subvolume

    int ret = mnt_context_mount(cxt);
    if (ret != 0)
	throw runtime_error(sformat("mnt_context_mount failed, ret:%d", ret));

    mnt_free_context(cxt);
    mnt_free_fs(x);
}


void
do_create_subvolume(const string& tmp_mountpoint, const string& subvolume_name)
{
    if (verbose)
	cout << "do-create-subvolume" << endl;

    string parent = tmp_mountpoint + "/" + dirname(subvolume_name);

    if (mkdir(parent.c_str(), 0777) != 0 && errno != EEXIST)
	throw runtime_error_with_errno("mkdir failed", errno);

    int fd = open(parent.c_str(), O_RDONLY);
    if (fd < 0)
	throw runtime_error_with_errno("open failed", errno);

    try
    {
	create_subvolume(fd, basename(subvolume_name));
    }
    catch(...)
    {
	close(fd);
	throw;
    }

    close(fd);
}


void
do_tmp_umount(const libmnt_fs* fs, const char* tmp_mountpoint)
{
    if (verbose)
	cout << "do-tmp-umount" << endl;

    system("/sbin/udevadm settle --timeout 20");

    libmnt_fs* x = mnt_copy_fs(NULL, fs);
    if (!x)
	throw runtime_error("mnt_copy_fs failed");

    struct libmnt_context* cxt = mnt_new_context();
    mnt_context_set_fs(cxt, x);
    mnt_context_set_target(cxt, tmp_mountpoint);

    int ret = mnt_context_umount(cxt);
    if (ret != 0)
	throw runtime_error(sformat("mnt_context_umount failed, ret:%d", ret));

    mnt_free_context(cxt);
    mnt_free_fs(x);
}


void
do_add_fstab_and_mount(MntTable& mnt_table, const libmnt_fs* fs, const string& subvol_option,
		       const string& subvolume_name)
{
    if (verbose)
	cout << "do-add-fstab-and-mount" << endl;

    libmnt_fs* x = mnt_copy_fs(NULL, fs);
    if (!x)
	throw runtime_error("mnt_copy_fs failed");

    mnt_fs_set_target(x, target.c_str());

    string full_subvol_option = subvolume_name;
    if (!subvol_option.empty())
	full_subvol_option.insert(0, subvol_option + "/");

    char* options = mnt_fs_strdup_options(x);
    mnt_optstr_remove_option(&options, "defaults");
    mnt_optstr_remove_option(&options, "ro");
    mnt_optstr_set_option(&options, "subvol", full_subvol_option.c_str());
    mnt_fs_set_options(x, options);
    free(options);

    // Caution: mnt_context_mount may change the source of x so the fstab
    // functions must be called first.
    mnt_table.add_fs(x);
    mnt_table.replace_file();

    if (mkdir(target.c_str(), 0777) != 0 && errno != EEXIST)
	throw runtime_error_with_errno("mkdir failed", errno);

    struct libmnt_context* cxt = mnt_new_context();
    mnt_context_set_fs(cxt, x);

    int ret = mnt_context_mount(cxt);
    if (ret != 0)
	throw runtime_error(sformat("mnt_context_mount failed, ret:%d", ret));

    mnt_free_context(cxt);

    mnt_free_fs(x);
}


void
do_set_nocow()
{
    if (!set_nocow)
	return;

    if (verbose)
	cout << "do-set-nocow" << endl;

    int fd = open(target.c_str(), O_RDONLY);
    if (fd == -1)
    {
	throw runtime_error_with_errno("open failed", errno);
    }

    unsigned long flags = 0;

    if (ioctl(fd, EXT2_IOC_GETFLAGS, &flags) == -1)
    {
	close(fd);
	throw runtime_error_with_errno("ioctl(EXT2_IOC_GETFLAGS) failed", errno);
    }

    flags |= FS_NOCOW_FL;

    if (ioctl(fd, EXT2_IOC_SETFLAGS, &flags) == -1)
    {
	close(fd);
	throw runtime_error_with_errno("ioctl(EXT2_IOC_SETFLAGS) failed", errno);
    }

    close(fd);
}


bool
is_subvol_mount(const string& fs_options)
{
    vector<string> tmp1;
    boost::split(tmp1, fs_options, boost::is_any_of(","), boost::token_compress_on);
    for (const string& tmp2 : tmp1)
    {
	if (boost::starts_with(tmp2, "subvol=") || boost::starts_with(tmp2, "subvolid="))
	    return true;
    }

    return false;
}


libmnt_fs*
find_filesystem(MntTable& mnt_table)
{
    string tmp = target;

    for (;;)
    {
	libmnt_fs* fs = mnt_table.find_target_up(tmp, MNT_ITER_FORWARD);
	if (!fs)
	    throw runtime_error("filesystem not found");

	string fs_device = mnt_fs_get_source(fs);
	string fs_fstype = mnt_fs_get_fstype(fs);
	string fs_target = mnt_fs_get_target(fs);
	string fs_options = mnt_fs_get_options(fs);

	if (verbose)
	{
	    cout << "fs-device:" << fs_device << endl;
	    cout << "fs-fstype:" << fs_fstype << endl;
	    cout << "fs-target:" << fs_target << endl;
	    cout << "fs-options:" << fs_options << endl;
	}

	if (fs_fstype != "btrfs")
	    throw runtime_error("filesystem is not btrfs");

	if (fs_target == target)
	    throw runtime_error("target exists in fstab");

	if (!is_subvol_mount(fs_options))
	    return fs;

	if (verbose)
	    cout << "ignoring subvol mount" << endl;

	if (tmp == "/")
	    throw runtime_error("filesystem not found");

	tmp = dirname(fs_target);
    }
}

struct TmpMountpoint {
    unique_ptr<char[]> mountpoint;
    const libmnt_fs* fs;
    TmpMountpoint(const string& tmpMountpoint, const libmnt_fs* libmntfs, const string& subvol_opts)
	: mountpoint(strdup(tmpMountpoint.c_str())), fs(libmntfs)
    {
	if (!mkdtemp(mountpoint.get()))
	    throw runtime_error_with_errno("mkdtemp failed", errno);

	try
	{
	    do_tmp_mount(fs, mountpoint.get(), subvol_opts);
	}
	catch (...)
	{
	    rmdir(mountpoint.get());
	    throw;
	}
    }

    ~TmpMountpoint()
    {
	do_tmp_umount(fs, mountpoint.get());
	rmdir(mountpoint.get());
    }
};


/*
 * The used algorithm is as follow:
 *
 * 1. Search upwards from target for a filesystem (not mounted with subvol
 *    option). The filesystem must of course be btrfs.
 *
 * 2. Determine the name for new subvolume: It is the target name without the
 *    leading filesystem target (mountpoint).
 *
 * 3. Temporarily mount the filesystem and create new subvolume. Reasons for
 *    the mount are documented in bsc #910602.
 *
 * 4. Create new subvolume.
 *
 * 5. Add new subvolume mount to fstab and mount new subvolume.
 */

void
doit()
{
    if (verbose)
	cout << "target:" << target << endl;

    if (target.empty() || boost::ends_with(target, "/"))
	throw runtime_error("invalid target");

    if (access(target.c_str(), F_OK) == 0)
	throw runtime_error("target exists");

    if (access(dirname(target).c_str(), F_OK) != 0)
	throw runtime_error("parent of target does not exist");

    MntTable mnt_table("/");
    mnt_table.parse_fstab();

    // Find filesystem.

    libmnt_fs* fs = find_filesystem(mnt_table);
    string fs_target = mnt_fs_get_target(fs);

    // Get default subvolume of filesystem.

    int fd = open(fs_target.c_str(), O_RDONLY);
    if (fd == -1)
	throw runtime_error_with_errno("open failed", errno);

    subvolid_t default_subvolume_id = get_default_id(fd);
    if (verbose)
	cout << "default-subvolume-id:" << default_subvolume_id << endl;

    string default_subvolume_name = get_subvolume(fd, default_subvolume_id);
    if (verbose)
	cout << "default-subvolume-name:" << default_subvolume_name << endl;

    close(fd);

    // Determine subvol mount-option for tmp mount. The '@' is used on SLE.

    string subvol_option = "";
    if (default_subvolume_name == "@" || boost::starts_with(default_subvolume_name, "@/"))
	subvol_option = "@";
    if (verbose)
	cout << "subvol-option:" << subvol_option << endl;

    // Determine name for new subvolume: It is the target name without the
    // leading filesystem target.

    string subvolume_name = target.substr(fs_target.size() + (fs_target == "/" ? 0 : 1));
    if (verbose)
	cout << "subvolume-name:" << subvolume_name << endl;

    // Execute all steps.

    TmpMountpoint tmp_mountpoint("/tmp/mksubvolume-XXXXXX", fs, subvol_option);

    try
    {
	do_create_subvolume(tmp_mountpoint.mountpoint.get(), subvolume_name);
    }
    catch (const runtime_error_with_errno& e)
    {
	if (e.error_number == EEXIST && force)
	{
	    const string path = string(tmp_mountpoint.mountpoint.get()).append("/").append(subvolume_name);

	    struct stat sb;
	    if (stat(path.c_str(), &sb) == 0 && !is_subvolume(sb))
		throw runtime_error_with_errno("path exists, but isn't a subvolume", e.error_number);
	    cout << "subvolume exists already, reusing" << endl;
	}
	else
	    throw;
    }

    do_add_fstab_and_mount(mnt_table, fs, subvol_option, subvolume_name);

    do_set_nocow();
}


void usage() __attribute__ ((__noreturn__));

void
usage()
{
    cerr << "usage: [--force] [--nocow] [--verbose] target" << endl;
    exit(EXIT_FAILURE);
}


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    const struct option options[] = {
	{ "force",		no_argument,		0,	'f' },
	{ "nocow",		no_argument,		0,	0 },
	{ "verbose",            no_argument,            0,      'v' },
	{ 0, 0, 0, 0 }
    };

    GetOpts getopts;

    getopts.init(argc, argv);

    GetOpts::parsed_opts opts = getopts.parse(options);

    GetOpts::parsed_opts::const_iterator opt;

    if ((opt = opts.find("force")) != opts.end())
        force = true;

    if ((opt = opts.find("nocow")) != opts.end())
        set_nocow = true;

    if ((opt = opts.find("verbose")) != opts.end())
        verbose = true;

    if (getopts.numArgs() != 1)
	usage();

    target = getopts.popArg();

    try
    {
	doit();
	exit(EXIT_SUCCESS);
    }
    catch (const std::exception& e)
    {
	cerr << "failure (" << e.what() << ")" << endl;
	exit(EXIT_FAILURE);
    }
}
