#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ambit

#include <boost/test/unit_test.hpp>
#include <fstream>
#include <cstdlib>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include "config.h"
#include "client/snapper/ambit.h"

using namespace std;
using namespace snapper;


// --- rootflags of the kernel command line ------------------------------------

struct CmdlineCase
{
    const char* label;
    const char* cmdline;
    vector<string> expected;
};


ostream& operator<<(ostream& os, const CmdlineCase& c)
{
    return os << c.label;
}


const CmdlineCase cmdline_cases[] = {
    { "fedora",        "BOOT_IMAGE=(hd0,gpt2)/vmlinuz root=UUID=x ro rootflags=subvol=root rhgb",
                       { "subvol=root" } },
    { "several_flags", "root=UUID=x rootflags=subvol=@,compress=zstd:1", { "subvol=@", "compress=zstd:1" } },
    { "quoted",        "root=UUID=x rootflags=\"subvol=@\" quiet", { "subvol=@" } },
    // several are joined, as systemd's fstab-generator does in the initrd
    { "joined",        "rootflags=subvol=a root=UUID=x rootflags=subvol=b", { "subvol=a", "subvol=b" } },
    { "arch",          "BOOT_IMAGE=/@/boot/vmlinuz-linux root=UUID=x rw rootflags=subvol=@ "
                       "net.ifnames=0 rootflags=compress=zstd:1 console=ttyS0,115200",
                       { "subvol=@", "compress=zstd:1" } },
    { "none",          "root=UUID=x ro quiet", { } },
    { "trailing_newline", "root=UUID=x rootflags=subvol=@\n", { "subvol=@" } },
    { "empty",         "", { } },
};


BOOST_DATA_TEST_CASE(cmdline_rootflags, boost::unit_test::data::make(cmdline_cases), c)
{
    const vector<string> got = rootflags_from_cmdline(c.cmdline);
    BOOST_CHECK_EQUAL_COLLECTIONS(got.begin(), got.end(), c.expected.begin(), c.expected.end());
}


// --- the root subvolume the system boots -----------------------------------

struct BootCase
{
    const char* label;
    vector<string> rootflags;		// from the kernel command line
    vector<string> fstab;		// options of the fstab entry for /
    const char* running;		// current name of the running root (/proc/mounts)
    const char* subvol;			// expected top-level name, "" = by default id
    bool unsupported;			// no rollback method changes what boots
    bool stale_subvolid;
};


ostream& operator<<(ostream& os, const BootCase& c)
{
    return os << c.label;
}


const BootCase boot_cases[] = {
    // the cmdline decides at boot, fstab mirrors it
    { "fedora",              { "subvol=root" }, { "subvol=root", "compress=zstd:1" }, "root", "root", false, false },
    { "debian",              { "subvol=@rootfs" }, { "defaults", "subvol=@rootfs" }, "@rootfs", "@rootfs", false, false },
    { "leading_slash",       { "subvol=/@" }, { }, "@", "@", false, false },
    { "arch",                { "subvol=@", "compress=zstd:1" }, { "rw", "subvol=/@" }, "@", "@", false, false },
    // after a rollback without a reboot the running root has another name
    { "renamed_running_root", { "subvol=root" }, { "subvol=root" }, "root.rollback.5", "root", false, false },
    // booted by the default subvolume id
    { "tumbleweed",          { }, { "defaults" }, "@/.snapshots/1/snapshot", "", false, false },
    { "tumbleweed_snapshot", { "subvol=@/.snapshots/7/snapshot" }, { "defaults" }, "@/.snapshots/7/snapshot", "", false, false },
    // a one-off boot of a snapshot (by id or by nested name): fstab has the normal root
    { "snapshot_by_id",      { "subvolid=271" }, { "subvol=@root" }, "@root/.snapshots/7/snapshot", "@root", false, false },
    { "snapshot_by_name",    { "subvol=@/.snapshots/7/snapshot" }, { "subvol=@" }, "@/.snapshots/7/snapshot", "@", false, false },
    { "fstab_only",          { }, { "subvol=@" }, "@", "@", false, false },
    // fstab additionally pins the id (e.g. written by genfstab): stale after a rename
    { "genfstab",            { "subvol=/@" }, { "rw", "subvolid=256", "subvol=/@" }, "@", "@", false, true },
    { "fstab_id_cmdline_name", { "subvol=root" }, { "subvolid=257" }, "root", "root", false, true },
    // pinned by id only: no rollback method changes what boots
    { "subvolid_only_fstab", { }, { "subvolid=256" }, "@", "", true, false },
    { "subvolid_only_both",  { "subvolid=256" }, { "subvolid=256" }, "@", "", true, false },
    // the cmdline pins the normal root by id permanently (not a snapshot boot)
    { "cmdline_pins_root_by_id", { "subvolid=256" }, { "subvol=@" }, "@", "", true, false },
    // cmdline and fstab name different subvolumes, e.g. a backup booted by hand:
    // which one boots next is unknown
    { "booted_backup_by_name", { "subvol=@.rollback.5" }, { "subvol=@" }, "@.rollback.5", "", true, false },
    { "cmdline_fstab_differ", { "subvol=@new" }, { "subvol=@old" }, "@new", "", true, false },
    // the root is mounted by a nested name: neither a rename nor set-default
    // changes what boots
    { "nested_in_fstab",     { }, { "subvol=@/root" }, "@/root", "", true, false },
    { "nested_both",         { "subvol=@/root" }, { "subvol=@/root" }, "@/root", "", true, false },
};


BOOST_DATA_TEST_CASE(boot_root, boost::unit_test::data::make(boot_cases), c)
{
    const BootRoot boot = boot_root_from(c.rootflags, c.fstab, c.running);
    BOOST_CHECK_EQUAL(boot.subvol, string(c.subvol));
    BOOST_CHECK_EQUAL(!boot.unsupported.empty(), c.unsupported);
    BOOST_CHECK_EQUAL(boot.stale_subvolid, c.stale_subvolid);
}


// fstab is read with libmount, so escaped characters in options are decoded
BOOST_AUTO_TEST_CASE(boot_root_fstab_escapes)
{
    char tmpl[] = "/tmp/ambit-test-XXXXXX";
    const string root = mkdtemp(tmpl);
    BOOST_REQUIRE_EQUAL(system(("mkdir -p " + root + "/etc").c_str()), 0);
    std::ofstream(root + "/etc/fstab") << "# comment\n"
	"UUID=1234 / btrfs subvol=@my\\040root,compress=zstd:1 0 0\n";

    const BootRoot boot = get_boot_root(root);
    BOOST_CHECK_EQUAL(boot.subvol, "@my root");

    BOOST_CHECK_EQUAL(system(("rm -rf " + root).c_str()), 0);
}


// --- determine_ambit -----------------------------------------------------------

struct AmbitCase
{
    const char* label;
    Ambit cli_ambit;			// --ambit (AUTO if not given)
    const char* subvol_name;		// named root subvolume ("" = default subvol id)
    SubvolumeMode mode;			// read-only/-write state of default snapshot
    Ambit expected;
};


ostream& operator<<(ostream& os, const AmbitCase& c)
{
    return os << c.label;
}


const AmbitCase ambit_cases[] = {
    // auto: a top-level named root subvolume selects subvol-rename
    { "auto_named",         Ambit::AUTO, "@root",      SubvolumeMode::READ_WRITE, Ambit::SUBVOL_RENAME },
    { "auto_named_ro",      Ambit::AUTO, "@root",      SubvolumeMode::READ_ONLY,  Ambit::SUBVOL_RENAME },
    { "auto_named_unknown", Ambit::AUTO, "@root",      SubvolumeMode::UNKNOWN,    Ambit::SUBVOL_RENAME },

    // auto: otherwise derive from the read-only/-write state of the default snapshot
    { "auto_rw",            Ambit::AUTO, "",           SubvolumeMode::READ_WRITE, Ambit::CLASSIC },
    { "auto_ro",            Ambit::AUTO, "",           SubvolumeMode::READ_ONLY,  Ambit::TRANSACTIONAL },
    { "auto_unknown",       Ambit::AUTO, "",           SubvolumeMode::UNKNOWN,    Ambit::AUTO },

    // auto: a nested subvolume cannot be renamed and falls back to set-default
    { "auto_nested_rw",     Ambit::AUTO, "root/@root", SubvolumeMode::READ_WRITE, Ambit::CLASSIC },
    { "auto_nested_ro",     Ambit::AUTO, "root/@root", SubvolumeMode::READ_ONLY,  Ambit::TRANSACTIONAL },

    // an explicit --ambit wins over any detection
    { "cli_classic",              Ambit::CLASSIC,       "",      SubvolumeMode::UNKNOWN,    Ambit::CLASSIC },
    { "cli_classic_over_ro",      Ambit::CLASSIC,       "",      SubvolumeMode::READ_ONLY,  Ambit::CLASSIC },
    { "cli_classic_over_named",   Ambit::CLASSIC,       "@root", SubvolumeMode::READ_WRITE, Ambit::CLASSIC },
    { "cli_trans_over_rw",        Ambit::TRANSACTIONAL, "",      SubvolumeMode::READ_WRITE, Ambit::TRANSACTIONAL },
    { "cli_trans_over_named",     Ambit::TRANSACTIONAL, "@root", SubvolumeMode::READ_WRITE, Ambit::TRANSACTIONAL },
    { "cli_rename_named",         Ambit::SUBVOL_RENAME, "@root", SubvolumeMode::UNKNOWN,    Ambit::SUBVOL_RENAME },
    { "cli_rename_named_ro",      Ambit::SUBVOL_RENAME, "@root", SubvolumeMode::READ_ONLY,  Ambit::SUBVOL_RENAME },
};


BOOST_DATA_TEST_CASE(ambit, boost::unit_test::data::make(ambit_cases), c)
{
    BOOST_CHECK(determine_ambit(c.cli_ambit, c.subvol_name, c.mode) == c.expected);
}


// subvol-rename requested where it cannot work must be rejected
BOOST_AUTO_TEST_CASE(rejects_rename_without_named_subvolume)
{
    BOOST_CHECK_THROW(determine_ambit(Ambit::SUBVOL_RENAME, "", SubvolumeMode::READ_WRITE), Exception);
    BOOST_CHECK_THROW(determine_ambit(Ambit::SUBVOL_RENAME, "root/@root", SubvolumeMode::READ_WRITE),
		      Exception);
}


// every Ambit value must have a name (keeps the enum and EnumInfo names in sync)
BOOST_AUTO_TEST_CASE(ambit_names_complete)
{
    BOOST_CHECK_EQUAL(toString(Ambit::AUTO), "auto");
    BOOST_CHECK_EQUAL(toString(Ambit::CLASSIC), "classic");
    BOOST_CHECK_EQUAL(toString(Ambit::TRANSACTIONAL), "transactional");
    BOOST_CHECK_EQUAL(toString(Ambit::SUBVOL_RENAME), "subvol-rename");
}


// --- is_set_default_ineffective -------------------------------------------------

struct IneffectiveCase
{
    const char* label;
    Ambit ambit;			// effective ambit of the rollback
    const char* subvol_name;		// named root subvolume ("" = default subvol id)
    bool expected;
};


ostream& operator<<(ostream& os, const IneffectiveCase& c)
{
    return os << c.label;
}


const IneffectiveCase ineffective_cases[] = {
    // set-default on a mount by a top-level subvol= name has no effect on the
    // next boot
    { "classic_named",         Ambit::CLASSIC,       "@root",      true },
    { "transactional_named",   Ambit::TRANSACTIONAL, "@root",      true },

    // no warning for a nested name: /proc/mounts also shows the resolved path
    // when mounted by the default subvolume id (e.g. on Tumbleweed), so a
    // nested name is no evidence of a by-name mount - set-default is the
    // normal working setup there
    { "classic_nested",        Ambit::CLASSIC,       "root/@root",              false },
    { "classic_tumbleweed",    Ambit::CLASSIC,       "@/.snapshots/1/snapshot", false },

    // set-default works when mounted by the default subvolume id
    { "classic_default",       Ambit::CLASSIC,       "",           false },
    { "transactional_default", Ambit::TRANSACTIONAL, "",           false },

    // subvol-rename does not use set-default at all
    { "rename_named",          Ambit::SUBVOL_RENAME, "@root",      false },
};


BOOST_DATA_TEST_CASE(ineffective, boost::unit_test::data::make(ineffective_cases), c)
{
    BOOST_CHECK_EQUAL(is_set_default_ineffective(c.ambit, c.subvol_name), c.expected);
}
