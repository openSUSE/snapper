/*
 * Copyright (c) [2026] SUSE LLC
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

#include <cerrno>
#include <cstdlib>
#include <sys/stat.h>
#include <linux/btrfs_tree.h>
#ifdef HAVE_LIBBTRFSUTIL
#include <btrfsutil.h>
#endif

#include "snapper/BtrfsNested.h"
#include "snapper/AppUtil.h"
#include "snapper/Exception.h"
#include "snapper/LoggerImpl.h"
#include "snapper/SnapperDefines.h"


namespace snapper
{

    namespace
    {

	// Opens the directory at the relative path (e.g. "var/lib") below
	// parent, one component at a time since SDir only takes plain names.
	SDir
	open_relative(const SDir& parent, const string& path)
	{
	    SDir dir = parent;
	    string::size_type pos = 0;
	    while (pos < path.size())
	    {
		string::size_type end = path.find('/', pos);
		if (end == string::npos)
		    end = path.size();
		dir = SDir(dir, path.substr(pos, end - pos));
		pos = end + 1;
	    }
	    return dir;
	}

    }


    NestedSubvolumes::NestedSubvolumes(const SDir& root_dir)
    {
#ifdef HAVE_LIBBTRFSUTIL
	struct btrfs_util_subvolume_iterator* iter;

	enum btrfs_util_error err = btrfs_util_create_subvolume_iterator_fd(root_dir.fd(), 0, 0,
									    &iter);
	if (err)
	    SN_THROW(IOErrorException("cannot list nested subvolumes: " +
				      string(btrfs_util_strerror(err))));

	// The iterator returns parents before their children, so a path below
	// one already collected is nested deeper (e.g. a snapshot in
	// .snapshots) and moves along with its parent.
	while (true)
	{
	    char* tmp;
	    err = btrfs_util_subvolume_iterator_next(iter, &tmp, nullptr);
	    if (err == BTRFS_UTIL_ERROR_STOP_ITERATION)
		break;
	    if (err)
	    {
		btrfs_util_destroy_subvolume_iterator(iter);
		SN_THROW(IOErrorException("cannot list nested subvolumes: " +
					  string(btrfs_util_strerror(err))));
	    }

	    const string path = tmp;
	    free(tmp);

	    bool deeper = false;
	    for (const string& parent : nested)
		if (path.compare(0, parent.size() + 1, parent + "/") == 0)
		    deeper = true;

	    if (!deeper)
		nested.push_back(path);
	}

	btrfs_util_destroy_subvolume_iterator(iter);
#else
	// Without it the nested subvolumes cannot be listed (nor deleted
	// recursively when pruning), so the rule cannot be applied.
	SN_THROW(IOErrorException("rollback refused: the subvol-rename ambit requires "
				  "snapper built with libbtrfsutil"));
#endif

	string paths;
	for (const string& path : nested)
	    paths += " " + path;
	y2mil("nested subvolumes:" << paths);
    }


    vector<string>
    NestedSubvolumes::check_empty(const SDir& root_dir) const
    {
	vector<string> empty;
	string not_empty;

	for (const string& path : nested)
	{
	    if (path == SNAPSHOTS_NAME)
		continue;

	    if (open_relative(root_dir, path).entries().empty())
		empty.push_back(path);
	    else
		not_empty += (not_empty.empty() ? "/" : ", /") + path;
	}

	// Data written after this check behaves like any other change made
	// after a rollback: it ends up in the old root.
	if (!not_empty.empty())
	    SN_THROW(IOErrorException("rollback refused, data in nested subvolumes would only "
				      "be reachable in the old root after the rollback: " +
				      not_empty + "; make them top-level subvolumes mounted "
				      "via fstab (like /home) or empty them"));

	return empty;
    }


    bool
    NestedSubvolumes::has_snapshots() const
    {
	for (const string& path : nested)
	    if (path == SNAPSHOTS_NAME)
		return true;
	return false;
    }


    void
    NestedSubvolumes::move_snapshots(const SDir& from_root, const SDir& to_root) const
    {
	if (!has_snapshots())
	    return;

	// the placeholder, or nothing if the target predates .snapshots
	if (to_root.rmdir(SNAPSHOTS_NAME) != 0 && errno != ENOENT)
	    SN_THROW(IOErrorException("rollback refused, cannot remove the " SNAPSHOTS_NAME
				      " placeholder from the new root: " + stringerror(errno)));

	if (from_root.rename(SNAPSHOTS_NAME, to_root, SNAPSHOTS_NAME) != 0)
	{
	    const int rename_errno = errno;
	    SN_THROW(IOErrorException("rollback refused, cannot move " SNAPSHOTS_NAME
				      " into the new root: " + stringerror(rename_errno) +
				      (rename_errno == EBUSY ? "; it is mounted, unmount it for "
				       "the rollback (it stays reachable as nested subvolume)" : "")));
	}

	y2mil("moved " SNAPSHOTS_NAME " into the new root");
    }


    vector<string>
    NestedSubvolumes::remove_placeholders(const SDir& new_root) const
    {
	vector<string> failed;

	for (const string& path : nested)
	{
	    if (path == SNAPSHOTS_NAME)
		continue;

	    // Only a placeholder is removed: the target snapshot may predate the
	    // nested subvolume (no entry, or a plain directory of its own).
	    const string::size_type slash = path.rfind('/');
	    const string name = slash == string::npos ? path : path.substr(slash + 1);

	    try
	    {
		const SDir parent = slash == string::npos ? new_root :
		    open_relative(new_root, path.substr(0, slash));

		struct stat st;
		if (parent.stat(name, &st, AT_SYMLINK_NOFOLLOW) != 0 ||
		    st.st_ino != BTRFS_EMPTY_SUBVOL_DIR_OBJECTID)
		    continue;

		if (parent.rmdir(name) != 0)
		{
		    const string msg = "cannot remove the placeholder of the nested subvolume /" +
			path + " from the new root: " + stringerror(errno) + "; nothing can be "
			"created there until it is removed with rmdir";
		    y2war(msg);
		    failed.push_back(msg);
		}
		else
		{
		    y2mil("removed placeholder " << path << " from new root");
		}
	    }
	    catch (const Exception& e)
	    {
		// the parent directory does not exist in the target snapshot
	    }
	}

	return failed;
    }

}
