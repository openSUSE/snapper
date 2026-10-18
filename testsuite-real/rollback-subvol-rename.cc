
#include "config.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/btrfs_tree.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "snapper/Btrfs.h"
#include "snapper/BtrfsUtils.h"
#include "snapper/BtrfsNested.h"
#include "snapper/FileUtils.h"
#include "snapper/SnapperTmpl.h"

using namespace std;
using namespace snapper;
using namespace BtrfsUtils;


static const string MNT = "/testsuite";
static const string TOP = MNT + "/top-mnt-rollback-test";


static void
die(const string& msg)
{
    cerr << "FAIL: " << msg << " (" << strerror(errno) << ")" << endl;
    exit(EXIT_FAILURE);
}


static void
run(const string& cmd)
{
    if (system(cmd.c_str()) != 0)
	die("command failed: " + cmd);
}


static string
find_device(const string& mount_point)
{
    FILE* fp = popen(("findmnt -n -o SOURCE " + mount_point).c_str(), "r");
    if (!fp) die("findmnt failed");
    char buf[256];
    string dev;
    if (fgets(buf, sizeof(buf), fp))
    {
	dev = buf;
	while (!dev.empty() && dev.back() == '\n')
	    dev.pop_back();
    }
    pclose(fp);
    if (dev.empty()) die("cannot find device for " + mount_point);
    return dev;
}


static void
check(bool cond, const string& msg)
{
    if (!cond)
    {
	cerr << "FAIL: " << msg << endl;
	exit(EXIT_FAILURE);
    }
}


static void
cleanup_subvol(SDir& parent, const char* name)
{
    struct stat st;
    if (parent.stat(name, &st, AT_SYMLINK_NOFOLLOW) == 0)
    {
	try { delete_subvolume(parent.fd(), name); }
	catch (...) {}
    }
}


static void
create_marker(SDir& parent, const char* name)
{
    int fd = parent.open(name, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) die(string("create ") + name);
    close(fd);
}


static bool
has_marker(SDir& parent, const char* name)
{
    struct stat st;
    return parent.stat(name, &st, AT_SYMLINK_NOFOLLOW) == 0;
}


static void
test_exchange_swaps_subvolume(SDir& top)
{
    cleanup_subvol(top, "@root");
    cleanup_subvol(top, "@root.snap");
    cleanup_subvol(top, "@root.incoming");
    cleanup_subvol(top, "@root.rollback.1");

    create_subvolume(top.fd(), "@root");
    {
	SDir root_sv(top, "@root");
	create_marker(root_sv, "original-marker");
    }

    {
	SDir root_sv(top, "@root");
	create_snapshot(root_sv.fd(), top.fd(), "@root.snap", true, no_qgroup);
    }

    {
	SDir root_sv(top, "@root");
	create_marker(root_sv, "post-snapshot-marker");
    }

    {
	SDir snap(top, "@root.snap");
	create_snapshot(snap.fd(), top.fd(), "@root.incoming", false, no_qgroup);
    }

    check(top.exchange("@root", "@root.incoming") == 0,
	  "exchange failed");

    check(top.rename("@root.incoming", "@root.rollback.1") == 0,
	  "rename old root failed");

    {
	SDir new_root(top, "@root");
	check(has_marker(new_root, "original-marker"),
	      "@root missing original-marker after rollback");
	check(!has_marker(new_root, "post-snapshot-marker"),
	      "@root still contains post-snapshot-marker after rollback");
    }

    {
	SDir old_root(top, "@root.rollback.1");
	check(has_marker(old_root, "original-marker"),
	      "@root.rollback.1 missing original-marker");
	check(has_marker(old_root, "post-snapshot-marker"),
	      "@root.rollback.1 missing post-snapshot-marker");
    }

    cout << "ok: rename-exchange-swaps-subvolume" << endl;

    cleanup_subvol(top, "@root.snap");
    cleanup_subvol(top, "@root");
    cleanup_subvol(top, "@root.rollback.1");
}


static void
test_rollback_name_collision_uses_subvolid_fallback(SDir& top)
{
    cleanup_subvol(top, "@root");
    cleanup_subvol(top, "@root.snap");
    cleanup_subvol(top, "@root.incoming");
    cleanup_subvol(top, "@root.rollback.1");

    create_subvolume(top.fd(), "@root");

    {
	SDir root_sv(top, "@root");
	create_snapshot(root_sv.fd(), top.fd(), "@root.snap", true, no_qgroup);
    }

    {
	SDir root_sv(top, "@root");
	create_marker(root_sv, "post-snapshot-marker");
    }

    {
	SDir snap(top, "@root.snap");
	create_snapshot(snap.fd(), top.fd(), "@root.incoming", false, no_qgroup);
    }

    check(top.exchange("@root", "@root.incoming") == 0,
	  "exchange failed");

    create_subvolume(top.fd(), "@root.rollback.1");

    subvolid_t svid;
    {
	SDir incoming_dir(top, "@root.incoming");
	svid = get_id(incoming_dir.fd());
    }
    const string alt_name = "@root.rollback.svid." + decString(svid);

    check(top.rename("@root.incoming", "@root.rollback.1") != 0,
	  "rename to @root.rollback.1 succeeded but it already existed");

    check(errno == EEXIST || errno == ENOTEMPTY,
	  string("expected EEXIST/ENOTEMPTY, got ") + strerror(errno));

    check(top.rename("@root.incoming", alt_name) == 0,
	  "fallback rename to " + alt_name + " failed");

    {
	SDir rescued(top, alt_name);
	check(has_marker(rescued, "post-snapshot-marker"),
	      alt_name + " missing post-snapshot-marker");
    }

    {
	SDir new_root(top, "@root");
	check(!has_marker(new_root, "post-snapshot-marker"),
	      "@root still contains post-snapshot-marker after rollback");
    }

    cout << "ok: rollback-name-collision-uses-subvolid-fallback (" << alt_name << ")" << endl;

    cleanup_subvol(top, "@root.snap");
    cleanup_subvol(top, "@root");
    cleanup_subvol(top, "@root.rollback.1");
    cleanup_subvol(top, alt_name.c_str());
}


// A btrfs snapshot only contains an empty stub directory where a nested
// subvolume is: verify the stub can be replaced by the old root's .snapshots
// subvolume with a cross-directory rename, as Btrfs::rollbackSubvolRename does.
static void
test_nested_snapshots_subvol_migrates(SDir& top)
{
    cleanup_subvol(top, "@root.snap");
    cleanup_subvol(top, "@root.incoming");
    cleanup_subvol(top, "@root");

    create_subvolume(top.fd(), "@root");
    {
	SDir root_sv(top, "@root");
	create_subvolume(root_sv.fd(), ".snapshots");
	SDir snapshots(root_sv, ".snapshots");
	create_marker(snapshots, "history-marker");
    }

    {
	SDir root_sv(top, "@root");
	create_snapshot(root_sv.fd(), top.fd(), "@root.snap", true, no_qgroup);
    }

    {
	SDir snap(top, "@root.snap");
	create_snapshot(snap.fd(), top.fd(), "@root.incoming", false, no_qgroup);
    }

    check(top.exchange("@root", "@root.incoming") == 0, "exchange failed");

    {
	SDir new_root(top, "@root");
	SDir old_root(top, "@root.incoming");

	struct stat st;
	check(new_root.stat(".snapshots", &st, AT_SYMLINK_NOFOLLOW) == 0,
	      "stub .snapshots missing in new root");
	check(!is_subvolume(st), "stub .snapshots is unexpectedly a subvolume");

	check(new_root.rmdir(".snapshots") == 0, "cannot remove stub .snapshots");
	check(old_root.rename(".snapshots", new_root, ".snapshots") == 0,
	      "cannot move .snapshots into new root");

	SDir snapshots(new_root, ".snapshots");
	check(has_marker(snapshots, "history-marker"),
	      ".snapshots content lost after migration");
    }

    {
	SDir new_root(top, "@root");
	try { delete_subvolume(new_root.fd(), ".snapshots"); } catch (...) {}
    }
    cleanup_subvol(top, "@root.snap");
    cleanup_subvol(top, "@root.incoming");
    cleanup_subvol(top, "@root");

    cout << "ok: nested-snapshots-subvol-migrates" << endl;
}


// Btrfs::rollbackSubvolRename preserves each old root as <subvol>.rollback.*.
// With ROLLBACK_BACKUP_LIMIT set, prune_rollback_backups keeps only the most
// recent ones and deletes older backups recursively (they contain nested
// subvolumes such as var/lib/portables).
static void
test_prune_keeps_recent_backups(SDir& top)
{
    for (const char* n : { "@root.rollback.1", "@root.rollback.2", "@root.rollback.3" })
    {
	// recursive: an earlier failed run may have left a nested subvolume
	try { delete_subvolume(top.fd(), n, true); } catch (...) {}
    }

    // Create three backups oldest-to-newest so their btrfs ids increase with
    // creation order (prune orders by id).
    create_subvolume(top.fd(), "@root.rollback.1");
    {
	// a nested subvolume inside the oldest backup: a non-recursive delete
	// would fail with ENOTEMPTY, so this proves the recursive delete.
	SDir b1(top, "@root.rollback.1");
	create_subvolume(b1.fd(), "nested");
    }
    create_subvolume(top.fd(), "@root.rollback.2");
    create_subvolume(top.fd(), "@root.rollback.3");

    // Keep the 2 most recent; the oldest (.1, with its nested subvolume) goes.
    prune_rollback_backups(top, "@root", 2, 0);

    struct stat st;
    check(top.stat("@root.rollback.1", &st, AT_SYMLINK_NOFOLLOW) != 0,
	  "oldest backup @root.rollback.1 not pruned (recursive delete of nested subvolume failed?)");
    check(top.stat("@root.rollback.2", &st, AT_SYMLINK_NOFOLLOW) == 0,
	  "@root.rollback.2 pruned but should be kept");
    check(top.stat("@root.rollback.3", &st, AT_SYMLINK_NOFOLLOW) == 0,
	  "newest backup @root.rollback.3 pruned but should be kept");

    // keep == 0 (the default) must keep everything.
    prune_rollback_backups(top, "@root", 0, 0);
    check(top.stat("@root.rollback.2", &st, AT_SYMLINK_NOFOLLOW) == 0 &&
	  top.stat("@root.rollback.3", &st, AT_SYMLINK_NOFOLLOW) == 0,
	  "keep == 0 must not delete any backup");

    cout << "ok: prune-keeps-recent-backups" << endl;

    try { delete_subvolume(top.fd(), "@root.rollback.2", true); } catch (...) {}
    try { delete_subvolume(top.fd(), "@root.rollback.3", true); } catch (...) {}
}


// The running root is never deleted but counts towards the limit: normally it
// is the newest backup, after two rollbacks without a reboot an older one.
static void
test_prune_keeps_running_root(SDir& top)
{
    for (const char* n : { "@root.rollback.1", "@root.rollback.2", "@root.rollback.3" })
	try { delete_subvolume(top.fd(), n, true); } catch (...) {}

    create_subvolume(top.fd(), "@root.rollback.1");
    create_subvolume(top.fd(), "@root.rollback.2");
    create_subvolume(top.fd(), "@root.rollback.3");

    subvolid_t running;
    {
	SDir running_dir(top, "@root.rollback.1");
	running = get_id(running_dir.fd());
    }

    prune_rollback_backups(top, "@root", 1, running);

    struct stat st;
    check(top.stat("@root.rollback.1", &st, AT_SYMLINK_NOFOLLOW) == 0,
	  "running root @root.rollback.1 was pruned");
    check(top.stat("@root.rollback.2", &st, AT_SYMLINK_NOFOLLOW) != 0 &&
	  top.stat("@root.rollback.3", &st, AT_SYMLINK_NOFOLLOW) != 0,
	  "the running root does not count towards the limit");

    // the normal case: the running root is the newest backup
    create_subvolume(top.fd(), "@root.rollback.4");
    create_subvolume(top.fd(), "@root.rollback.5");
    {
	SDir running_dir(top, "@root.rollback.5");
	running = get_id(running_dir.fd());
    }
    prune_rollback_backups(top, "@root", 2, running);
    check(top.stat("@root.rollback.1", &st, AT_SYMLINK_NOFOLLOW) != 0 &&
	  top.stat("@root.rollback.4", &st, AT_SYMLINK_NOFOLLOW) == 0 &&
	  top.stat("@root.rollback.5", &st, AT_SYMLINK_NOFOLLOW) == 0,
	  "limit 2 with the running root as newest backup does not keep exactly 4 and 5");

    cout << "ok: prune-keeps-running-root" << endl;

    for (const char* n : { "@root.rollback.4", "@root.rollback.5" })
	try { delete_subvolume(top.fd(), n, true); } catch (...) {}
}


// A backup that still holds the .snapshots subvolume (moving it into the new
// root failed) holds all snapshots: the prune must never delete it.
static void
test_prune_keeps_backup_with_snapshots(SDir& top)
{
    for (const char* n : { "@root.rollback.1", "@root.rollback.2", "@root.rollback.3" })
	try { delete_subvolume(top.fd(), n, true); } catch (...) {}

    create_subvolume(top.fd(), "@root.rollback.1");
    {
	SDir b1(top, "@root.rollback.1");
	create_subvolume(b1.fd(), ".snapshots");
	SDir snapshots(b1, ".snapshots");
	snapshots.mkdir("1", 0755);
	SDir one(snapshots, "1");
	create_subvolume(one.fd(), "snapshot");
    }
    create_subvolume(top.fd(), "@root.rollback.2");
    create_subvolume(top.fd(), "@root.rollback.3");

    prune_rollback_backups(top, "@root", 1, 0);

    struct stat st;
    check(top.stat("@root.rollback.1", &st, AT_SYMLINK_NOFOLLOW) == 0,
	  "backup @root.rollback.1 holding .snapshots was pruned");

    // it counts towards the limit (1), so the others go
    check(top.stat("@root.rollback.2", &st, AT_SYMLINK_NOFOLLOW) != 0 &&
	  top.stat("@root.rollback.3", &st, AT_SYMLINK_NOFOLLOW) != 0,
	  "the backup holding .snapshots does not count towards the limit");

    cout << "ok: prune-keeps-backup-with-snapshots" << endl;

    try { delete_subvolume(top.fd(), "@root.rollback.1", true); } catch (...) {}
}


// Opens the directory at the relative path (e.g. "var/lib") below parent.
static SDir
open_path(const SDir& parent, const string& path)
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


// Creates the directories of the relative path below parent, like mkdir -p.
static void
mkdirs(const SDir& parent, const string& path)
{
    SDir dir = parent;
    string::size_type pos = 0;
    while (pos < path.size())
    {
	string::size_type end = path.find('/', pos);
	if (end == string::npos)
	    end = path.size();
	const string name = path.substr(pos, end - pos);
	dir.mkdir(name, 0755);
	dir = SDir(dir, name);
	pos = end + 1;
    }
}


// Creates a subvolume at the relative path (e.g. "var/lib/machines") below
// parent, creating the directories leading to it.
static void
create_nested(const SDir& parent, const string& path)
{
    const string::size_type slash = path.rfind('/');
    if (slash == string::npos)
    {
	create_subvolume(parent.fd(), path);
	return;
    }
    mkdirs(parent, path.substr(0, slash));
    SDir dir = open_path(parent, path.substr(0, slash));
    create_subvolume(dir.fd(), path.substr(slash + 1));
}


static ino_t
inode_at(const SDir& parent, const string& path)
{
    const string::size_type slash = path.rfind('/');
    SDir dir = slash == string::npos ? parent : open_path(parent, path.substr(0, slash));
    struct stat st;
    if (dir.stat(slash == string::npos ? path : path.substr(slash + 1), &st,
		 AT_SYMLINK_NOFOLLOW) != 0)
	return 0;
    return st.st_ino;
}


// Builds @root with the nested subvolumes of a stock Fedora install.
static void
create_nested_root(SDir& top)
{
    for (const char* n : { "@root", "@root.snap", "@root.incoming" })
	try { delete_subvolume(top.fd(), n, true); } catch (...) {}

    create_subvolume(top.fd(), "@root");
    SDir root_sv(top, "@root");
    create_nested(root_sv, ".snapshots");
    create_nested(root_sv, ".snapshots/1");	// plain directory, as snapper does
    create_nested(root_sv, ".snapshots/1/snapshot");
    create_nested(root_sv, "var/lib/machines");
    create_nested(root_sv, "var/lib/machines/inner");
    create_nested(root_sv, "var/lib/portables");
    mkdirs(root_sv, "opt");
}


static void
cleanup_nested_root(SDir& top)
{
    for (const char* n : { "@root", "@root.snap", "@root.incoming" })
	try { delete_subvolume(top.fd(), n, true); } catch (...) {}
}


// Only the subvolumes nested directly in the root count: one inside another
// nested subvolume (e.g. a snapshot in .snapshots) moves along with it.
static void
test_nested_inventory(SDir& top)
{
    create_nested_root(top);

    vector<string> paths;
    {
	SDir root_sv(top, "@root");
	paths = NestedSubvolumes(root_sv).paths();
    }
    sort(paths.begin(), paths.end());

    const vector<string> expected = { ".snapshots", "var/lib/machines", "var/lib/portables" };
    string got;
    for (const string& path : paths)
	got += " " + path;
    check(paths == expected, "nested subvolume inventory is" + got);

    cleanup_nested_root(top);

    cout << "ok: nested-inventory" << endl;
}


// Empty nested subvolumes and the content of .snapshots do not block the
// rollback; a nested subvolume with data does, naming its path.
static void
test_nested_check_empty(SDir& top)
{
    create_nested_root(top);

    SDir root_sv(top, "@root");
    {
	SDir snapshots(root_sv, ".snapshots");
	create_marker(snapshots, "history-marker");
    }
    {
	SDir machines = open_path(root_sv, "var/lib/machines");
	delete_subvolume(machines.fd(), "inner");
    }

    NestedSubvolumes nested(root_sv);

    bool threw = false;
    try { nested.check_empty(root_sv); }
    catch (const std::exception& e) { threw = true; cerr << e.what() << endl; }
    check(!threw, "check_empty refused empty nested subvolumes");

    {
	SDir portables = open_path(root_sv, "var/lib/portables");
	create_marker(portables, "image.raw");
    }

    string what;
    try { nested.check_empty(root_sv); }
    catch (const std::exception& e) { what = e.what(); }
    check(!what.empty(), "check_empty accepted a nested subvolume with data");
    check(what.find("var/lib/portables") != string::npos,
	  "check_empty error does not name var/lib/portables: " + what);
    check(what.find("var/lib/machines") == string::npos,
	  "check_empty error names the empty var/lib/machines: " + what);

    cleanup_nested_root(top);

    cout << "ok: nested-check-empty" << endl;
}


// The copy of the target has placeholders where the nested subvolumes were:
// .snapshots moves in before the exchange, the other placeholders go away
// after it.
static void
test_nested_apply(SDir& top)
{
    create_nested_root(top);

    {
	SDir root_sv(top, "@root");
	SDir machines = open_path(root_sv, "var/lib/machines");
	delete_subvolume(machines.fd(), "inner");

	SDir snapshots(root_sv, ".snapshots");
	create_marker(snapshots, "history-marker");

	// a plain directory with data in the target snapshot ...
	mkdirs(root_sv, "srv/data");
	SDir data = open_path(root_sv, "srv/data");
	create_marker(data, "kept-marker");
    }

    {
	SDir root_sv(top, "@root");
	create_snapshot(root_sv.fd(), top.fd(), "@root.snap", true, no_qgroup);
    }

    {
	SDir root_sv(top, "@root");

	// ... that became a subvolume afterwards: the new root keeps the
	// directory of the target snapshot.
	SDir srv(root_sv, "srv");
	SDir data(srv, "data");
	data.unlink("kept-marker", 0);
	srv.rmdir("data");
	create_subvolume(srv.fd(), "data");

	// a nested subvolume created after the target snapshot: no entry at
	// all in the new root
	create_nested(root_sv, "var/lib/late");
    }

    {
	SDir snap(top, "@root.snap");
	create_snapshot(snap.fd(), top.fd(), "@root.incoming", false, no_qgroup);
    }

    {
	SDir root_sv(top, "@root");
	NestedSubvolumes nested(root_sv);

	check(nested.has_snapshots(), "has_snapshots is false");
	nested.move_snapshots(root_sv, SDir(top, "@root.incoming"));
	check(top.exchange("@root", "@root.incoming") == 0, "exchange failed");

	SDir new_root(top, "@root");
	SDir old_root(top, "@root.incoming");

	check(inode_at(new_root, "var/lib/machines") == BTRFS_EMPTY_SUBVOL_DIR_OBJECTID,
	      "test setup: var/lib/machines is no placeholder in the new root");

	check(nested.remove_placeholders(new_root).empty(), "remove_placeholders failed");

	check(inode_at(new_root, ".snapshots") == BTRFS_FIRST_FREE_OBJECTID,
	      ".snapshots is not a subvolume in the new root");
	{
	    SDir snapshots(new_root, ".snapshots");
	    check(has_marker(snapshots, "history-marker"), ".snapshots content lost");
	}

	check(inode_at(new_root, "var/lib/machines") == 0,
	      "placeholder var/lib/machines not removed from the new root");
	check(inode_at(new_root, "var/lib/portables") == 0,
	      "placeholder var/lib/portables not removed from the new root");
	check(inode_at(new_root, "var/lib/late") == 0,
	      "var/lib/late unexpectedly in the new root");
	check(inode_at(new_root, "var/lib") != 0, "var/lib missing in the new root");

	{
	    SDir data = open_path(new_root, "srv/data");
	    check(has_marker(data, "kept-marker"),
		  "directory srv/data of the target snapshot was changed");
	}

	check(inode_at(old_root, "var/lib/machines") == BTRFS_FIRST_FREE_OBJECTID,
	      "var/lib/machines no longer in the old root");
	check(inode_at(old_root, ".snapshots") == 0, ".snapshots still in the old root");
    }

    cleanup_nested_root(top);

    cout << "ok: nested-apply" << endl;
}


// A mounted .snapshots cannot be renamed (EBUSY): the move fails before the
// exchange, naming the reason, with .snapshots still in the root.
static void
test_nested_move_snapshots_mounted(SDir& top)
{
    create_nested_root(top);
    {
	SDir root_sv(top, "@root");
	create_snapshot(root_sv.fd(), top.fd(), "@root.incoming", false, no_qgroup);
    }

    const string snapshots = TOP + "/@root/.snapshots";
    run("mount --bind " + snapshots + " " + snapshots);
    string what;
    {
	SDir root_sv(top, "@root");
	try { NestedSubvolumes(root_sv).move_snapshots(root_sv, SDir(top, "@root.incoming")); }
	catch (const std::exception& e) { what = e.what(); }
    }
    run("umount " + snapshots);

    check(what.find("mounted") != string::npos, "move of a mounted .snapshots: '" + what + "'");
    {
	SDir root_sv(top, "@root");
	check(inode_at(root_sv, ".snapshots") == BTRFS_FIRST_FREE_OBJECTID,
	      ".snapshots left the root");
    }

    cleanup_nested_root(top);

    cout << "ok: nested-move-snapshots-mounted" << endl;
}


// A placeholder that cannot be removed is reported, naming the path: here
// its parent directory is immutable.
static void
test_nested_placeholder_failure(SDir& top)
{
    create_nested_root(top);
    {
	SDir root_sv(top, "@root");
	create_snapshot(root_sv.fd(), top.fd(), "@root.incoming", false, no_qgroup);
    }

    const string var_lib = TOP + "/@root.incoming/var/lib";
    run("chattr +i " + var_lib);
    vector<string> failed;
    {
	SDir root_sv(top, "@root");
	failed = NestedSubvolumes(root_sv).remove_placeholders(SDir(top, "@root.incoming"));
    }
    run("chattr -i " + var_lib);

    string got;
    for (const string& msg : failed)
	got += " [" + msg + "]";
    check(failed.size() == 2 && got.find("/var/lib/machines") != string::npos &&
	  got.find("/var/lib/portables") != string::npos, "failed placeholders:" + got);

    cleanup_nested_root(top);

    cout << "ok: nested-placeholder-failure" << endl;
}


#ifndef HAVE_LIBBTRFSUTIL

// Without libbtrfsutil the nested subvolumes cannot be listed (nor deleted
// recursively), so the subvol-rename rollback must be refused.
static void
test_nested_requires_libbtrfsutil(SDir& top)
{
    try { delete_subvolume(top.fd(), "@root"); } catch (...) {}
    create_subvolume(top.fd(), "@root");

    string what;
    try
    {
	SDir root_sv(top, "@root");
	NestedSubvolumes nested(root_sv);
    }
    catch (const std::exception& e)
    {
	what = e.what();
    }

    try { delete_subvolume(top.fd(), "@root"); } catch (...) {}

    check(what.find("libbtrfsutil") != string::npos,
	  "NestedSubvolumes without libbtrfsutil did not refuse: '" + what + "'");

    cout << "ok: nested-requires-libbtrfsutil" << endl;
}

#endif


int
main()
{
    if (getuid() != 0)
    {
	cerr << "This test must be run as root." << endl;
	return EXIT_FAILURE;
    }

    const string device = find_device(MNT);

    run("mkdir -p " + TOP);
    run("mount -o subvolid=5 " + device + " " + TOP);

    {
	SDir top(TOP);

	test_exchange_swaps_subvolume(top);
	test_rollback_name_collision_uses_subvolid_fallback(top);
	test_nested_snapshots_subvol_migrates(top);
#ifdef HAVE_LIBBTRFSUTIL
	// the prune deletes recursively and the nested subvolume handling lists
	// them, both only with libbtrfsutil (without, the rollback is refused)
	test_prune_keeps_recent_backups(top);
	test_prune_keeps_running_root(top);
	test_prune_keeps_backup_with_snapshots(top);
	test_nested_inventory(top);
	test_nested_check_empty(top);
	test_nested_apply(top);
	test_nested_move_snapshots_mounted(top);
	test_nested_placeholder_failure(top);
#else
	test_nested_requires_libbtrfsutil(top);
#endif

    }

    run("umount " + TOP);
    run("rmdir " + TOP);

    return EXIT_SUCCESS;
}
