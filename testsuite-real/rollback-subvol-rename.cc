
#include "config.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>

#include "client/snapper/ambit.h"
#include "snapper/Btrfs.h"
#include "snapper/BtrfsUtils.h"
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
    prune_rollback_backups(top, "@root", 2);

    struct stat st;
    check(top.stat("@root.rollback.1", &st, AT_SYMLINK_NOFOLLOW) != 0,
	  "oldest backup @root.rollback.1 not pruned (recursive delete of nested subvolume failed?)");
    check(top.stat("@root.rollback.2", &st, AT_SYMLINK_NOFOLLOW) == 0,
	  "@root.rollback.2 pruned but should be kept");
    check(top.stat("@root.rollback.3", &st, AT_SYMLINK_NOFOLLOW) == 0,
	  "newest backup @root.rollback.3 pruned but should be kept");

    // keep == 0 (the default) must keep everything.
    prune_rollback_backups(top, "@root", 0);
    check(top.stat("@root.rollback.2", &st, AT_SYMLINK_NOFOLLOW) == 0 &&
	  top.stat("@root.rollback.3", &st, AT_SYMLINK_NOFOLLOW) == 0,
	  "keep == 0 must not delete any backup");

    cout << "ok: prune-keeps-recent-backups" << endl;

    try { delete_subvolume(top.fd(), "@root.rollback.2", true); } catch (...) {}
    try { delete_subvolume(top.fd(), "@root.rollback.3", true); } catch (...) {}
}


static void
test_detect_method_strips_kernel_leading_slash(SDir& top, const string& device,
					       const string& sv_name)
{
    const string mnt_point = MNT + "/detect-test";

    create_subvolume(top.fd(), sv_name.c_str());

    run("mkdir -p " + mnt_point);
    run("mount -o subvol=" + sv_name + " " + device + " " + mnt_point);

    string subvol_name = get_subvol_name(mnt_point);
    bool rename = determine_ambit(Ambit::AUTO, subvol_name, SubvolumeMode::UNKNOWN) ==
	Ambit::SUBVOL_RENAME;

    run("umount " + mnt_point);
    run("rmdir " + mnt_point);
    delete_subvolume(top.fd(), sv_name.c_str());

    check(rename,
	  "determine_ambit for subvol=" + sv_name + " did not select subvol-rename");

    check(subvol_name == sv_name,
	  "get_subvol_name for subvol=" + sv_name +
	  " returned '" + subvol_name + "', expected '" + sv_name + "'");

    cout << "ok: detect-method-strips-leading-slash subvol=" << sv_name << endl;
}


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
	test_prune_keeps_recent_backups(top);

	const vector<string> subvol_names = { "@root", "root", "@rootfs", "@" };
	for (const string& name : subvol_names)
	    test_detect_method_strips_kernel_leading_slash(top, device, name);
    }

    run("umount " + TOP);
    run("rmdir " + TOP);

    return EXIT_SUCCESS;
}
