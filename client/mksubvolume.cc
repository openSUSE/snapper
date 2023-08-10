/*
 * Copyright (c) [2015-2020] SUSE LLC
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


#include <cstring>
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

bool set_nocow = false;
bool verbose = false;


class TmpMountpoint
{

public:

    TmpMountpoint(const libmnt_fs* fs, const string& subvol_opts);
    ~TmpMountpoint();

    const string& get_path() const { return path; }

private:

    void do_tmp_mount(const string& subvol_option) const;
    void do_tmp_umount() const;

    const libmnt_fs* fs;
    string path;

};


TmpMountpoint::TmpMountpoint(const libmnt_fs* fs, const string& subvol_opts)
    : fs(fs)
{
    char tmp[] = "/tmp/mksubvolume-XXXXXX";

    if (mkdtemp(tmp) == nullptr)
	throw runtime_error_with_errno("mkdtemp failed", errno);

    path = tmp;

    if (verbose)
	cout << "tmp directory is " << path << endl;

    try
    {
	do_tmp_mount(subvol_opts);
    }
    catch (...)
    {
	rmdir(path.c_str());
	throw;
    }
}


TmpMountpoint::~TmpMountpoint()
{
    try
    {
	do_tmp_umount();
	rmdir(path.c_str());
    }
    catch (...)
    {
	cerr << "failed to unmount and remove tmp directory " << path << endl;
    }
}


void
TmpMountpoint::do_tmp_mount(const string& subvol_option) const
{
    if (verbose)
	cout << "do-tmp-mount" << endl;

    libmnt_fs* x = mnt_copy_fs(NULL, fs);
    if (!x)
	throw runtime_error("mnt_copy_fs failed");

    struct libmnt_context* cxt = mnt_new_context();
    mnt_context_set_fs(cxt, x);
    mnt_context_set_target(cxt, path.c_str());
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
TmpMountpoint::do_tmp_umount() const
{
    if (verbose)
	cout << "do-tmp-umount" << endl;

    system("/usr/bin/udevadm settle --timeout 20");

    libmnt_fs* x = mnt_copy_fs(NULL, fs);
    if (!x)
	throw runtime_error("mnt_copy_fs failed");

    struct libmnt_context* cxt = mnt_new_context();
    mnt_context_set_fs(cxt, x);
    mnt_context_set_target(cxt, path.c_str());

    int ret = mnt_context_umount(cxt);
    if (ret != 0)
	throw runtime_error(sformat("mnt_context_umount failed, ret:%d", ret));

    mnt_free_context(cxt);
    mnt_free_fs(x);
}


void
do_create_subvolume_pure(const string& tmp_mountpoint, const string& subvolume_name)
{
    if (verbose)
	cout << "do-create-subvolume" << endl;

    string parent = tmp_mountpoint + "/" + dirname(subvolume_name);

    if (mkdir(parent.c_str(), 0777) != 0 && errno != EEXIST)
	throw runtime_error_with_errno("mkdir failed", errno);

    int fd = open(parent.c_str(), O_RDONLY);
    if (fd < 0)
	throw runtime_error_with_errno("open failed", errno);

    FdCloser fd_closer(fd);

    create_subvolume(fd, basename(subvolume_name));
}


void
do_create_subvolume(const string& subvolume_name, const libmnt_fs* fs, const string& subvol_option)
{
    // Note: If the target is /tmp it is important that the tmp mount is unmounted before
    // the subvolume itself is mounted.

    TmpMountpoint tmp_mountpoint(fs, subvol_option);

    try
    {
	do_create_subvolume_pure(tmp_mountpoint.get_path(), subvolume_name);
    }
    catch (const runtime_error_with_errno& e)
    {
	if (e.error_number != EEXIST)
	    throw;

	const string path = tmp_mountpoint.get_path() + "/" + subvolume_name;

	struct stat sb;
	if (lstat(path.c_str(), &sb) == 0 && is_subvolume(sb))
	    cout << "reusing existing subvolume" << endl;
	else
	    throw runtime_error_with_errno("cannot reuse path as subvolume", e.error_number);
    }
}


libmnt_fs*
create_fstab_line(const libmnt_fs* fs, const string& subvol_option,
		  const string& subvolume_name)
{
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

    return x;
}


void
do_add_fstab_and_mount(MntTable& mnt_table, libmnt_fs* x)
{
    if (verbose)
	cout << "do-add-fstab-and-mount" << endl;

    if (mnt_table.find_target(target.c_str(), MNT_ITER_FORWARD) == NULL)
    {
	// Caution: mnt_context_mount may change the source of x so the fstab
	// functions must be called first.
	mnt_table.add_fs(x);
	mnt_table.replace_file();
    }
    else
	cout << "reusing existing fstab entry" << endl;

    if (mkdir(target.c_str(), 0777) != 0 && errno != EEXIST)
	throw runtime_error_with_errno("mkdir failed", errno);

    struct libmnt_context* cxt = mnt_new_context();
    libmnt_fs* y;
    if (mnt_context_find_umount_fs(cxt, target.c_str(), &y))
    {
	mnt_context_set_fs(cxt, x);

	int ret = mnt_context_mount(cxt);
	if (ret != 0)
	    throw runtime_error(sformat("mnt_context_mount failed, ret:%d", ret));
    }
    else
	cout << "reusing mounted target" << endl;

    mnt_free_context(cxt);

    mnt_free_fs(x);
}


void
do_set_cow_flag()
{
    if (verbose)
	cout << "do-set-cow-flag" << endl;

    int fd = open(target.c_str(), O_RDONLY);
    if (fd == -1)
	throw runtime_error_with_errno("open failed", errno);

    FdCloser fd_closer(fd);

    unsigned long flags = 0;

    if (ioctl(fd, EXT2_IOC_GETFLAGS, &flags) == -1)
	throw runtime_error_with_errno("ioctl(EXT2_IOC_GETFLAGS) failed", errno);

    if (set_nocow)
	flags |= FS_NOCOW_FL;
    else
	flags &= ~FS_NOCOW_FL;

    if (ioctl(fd, EXT2_IOC_SETFLAGS, &flags) == -1)
	throw runtime_error_with_errno("ioctl(EXT2_IOC_SETFLAGS) failed", errno);
}


bool
is_subvol_mount(libmnt_fs* fs)
{
    if (mnt_fs_get_option(fs, "subvol", NULL, NULL) == 0)
	return true;
    if (mnt_fs_get_option(fs, "subvolid", NULL, NULL) == 0)
	return true;
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

	if (!is_subvol_mount(fs))
	    return fs;

	if (verbose)
	    cout << "ignoring subvol mount" << endl;

	if (tmp == "/")
	    throw runtime_error("filesystem not found");

	tmp = dirname(fs_target);
    }
}


string
get_abs_subvol_path(string subvolume)
{
    if(!boost::starts_with(subvolume, "/"))
	subvolume.insert(0, "/");
    return subvolume;
}


void
do_consistency_checks(MntTable& mnt_table, const libmnt_fs* fs, libmnt_fs* expected_fs)
{
    // Set up cache for UUID / LABEL resolution in mnt_table_find_source
    libmnt_cache* cache = mnt_new_cache();
    MntTable mtab_table("/");
    mtab_table.set_cache(cache);
    mtab_table.parse_mtab();

    char* subvol_expected;
    if (mnt_fs_get_option(expected_fs, "subvol", &subvol_expected, NULL) != 0)
	throw runtime_error("mnt_fs_get_option failed");

    // Consistency checks on (partially) existing entries
    libmnt_fs* fstab_entry = mnt_table.find_target(target, MNT_ITER_FORWARD);
    libmnt_fs* mounted_entry = mtab_table.find_target(mnt_fs_get_target(expected_fs),
						      MNT_ITER_BACKWARD);
    // Map UUID / LABEL to a physical device name
    const char* dev_expected = mnt_resolve_spec(mnt_fs_get_source(expected_fs), cache);
    const char* dev_fstab = mnt_resolve_spec(mnt_fs_get_source(fstab_entry), cache);
    if (dev_expected == NULL)
	throw runtime_error("parent volume in fstab does not match expected device");
    if (fstab_entry != NULL && dev_fstab == NULL)
	throw runtime_error("fstab entry does not map to a real device");
    if (fstab_entry != NULL && strcmp(dev_fstab, dev_expected) != 0)
	throw runtime_error("existing fstab entry doesn't match target device");
    if (fstab_entry != NULL)
    {
	char* subvol_fstab;
	if (mnt_fs_get_option(fstab_entry, "subvol", &subvol_fstab, NULL) != 0 ||
		get_abs_subvol_path(subvol_fstab) != get_abs_subvol_path(subvol_expected))
	    throw runtime_error("existing fstab entry's subvolume doesn't match");
    }

    // Something is mounted there already. Is it the correct device?
    if (mounted_entry != NULL)
    {
	if (strcmp(dev_expected, mnt_fs_get_source(mounted_entry)) != 0)
	{
	    // In case of multi device btrfs the device name can differ, so compare the UUIDs.
	    const char* uuid_expected = mnt_cache_find_tag_value(cache, dev_expected, "UUID");
	    const char* uuid_mounted = mnt_cache_find_tag_value(cache, mnt_fs_get_source(mounted_entry), "UUID");

	    if (!uuid_expected || !uuid_mounted)
		throw runtime_error("failed to get uuid");

	    if (strcmp(uuid_expected, uuid_mounted) != 0)
		throw runtime_error("different device mounted on target");
	}

	char* subvol_real;
	if (mnt_fs_get_option(mounted_entry, "subvol", &subvol_real, NULL) != 0 ||
	    get_abs_subvol_path(subvol_expected) != get_abs_subvol_path(subvol_real))
	    throw runtime_error("subvolume of mounted target doesn't match");
    }

    mnt_unref_cache(cache);
}


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
    {
	struct stat sb;
	if (lstat(target.c_str(), &sb) == 0 && sb.st_mode & S_IFDIR)
	    cout << "reusing existing target dir" << endl;
	else
	    throw runtime_error("target exists");
    }

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

    FdCloser fd_closer(fd);

    subvolid_t default_subvolume_id = get_default_id(fd);
    if (verbose)
	cout << "default-subvolume-id:" << default_subvolume_id << endl;

    string default_subvolume_name = get_subvolume(fd, default_subvolume_id);
    if (verbose)
	cout << "default-subvolume-name:" << default_subvolume_name << endl;

    fd_closer.close();

    // Determine subvol mount-option for tmp mount. The '@' is used on SLE.

    string subvol_option = "";
    if (default_subvolume_name == "@" || boost::starts_with(default_subvolume_name, "@/"))
	subvol_option = "@";
    if (verbose)
	cout << "subvol-option:" << subvol_option << endl;

    // Determine name for new subvolume: It is the target name without the
    // leading filesystem target.

    string subvolume_name = target.substr(fs_target.size() +
					 (fs_target == "/" || fs_target == target ? 0 : 1));
    if (subvolume_name.empty())
	throw runtime_error("target is a dedicated mountpoint");
    if (verbose)
	cout << "subvolume-name:" << subvolume_name << endl;

    // Create the new subvolume in memory and check system environment

    libmnt_fs* expected_fs = create_fstab_line(fs, subvol_option, subvolume_name);
    do_consistency_checks(mnt_table, fs, expected_fs);

    // Execute all steps.

    do_create_subvolume(subvolume_name, fs, subvol_option);

    do_add_fstab_and_mount(mnt_table, expected_fs);

    do_set_cow_flag();
}


void usage() __attribute__ ((__noreturn__));

void
usage()
{
    cerr << "usage: [--nocow] [--verbose] target" << endl;
    exit(EXIT_FAILURE);
}


int
main(int argc, char** argv)
{
    setlocale(LC_ALL, "");

    try
    {
	const vector<Option> options = {
	    Option("nocow",		no_argument),
	    Option("verbose",		no_argument,	'v'),
	};

	GetOpts get_opts(argc, argv);

	ParsedOpts opts = get_opts.parse(options);

	if (opts.has_option("nocow"))
	    set_nocow = true;

	if (opts.has_option("verbose"))
	    verbose = true;

	if (get_opts.num_args() != 1)
	    usage();

	target = get_opts.pop_arg();
    }
    catch (const OptionsException& e)
    {
	SN_CAUGHT(e);
	cerr << e.what() << endl;
	usage();
    }

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
