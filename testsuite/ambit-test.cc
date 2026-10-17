#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE ambit

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include "config.h"
#include "client/snapper/ambit.h"

using namespace std;
using namespace snapper;


// --- subvolume name parsing ---------------------------------------------------

struct SubvolCase
{
    const char* label;
    vector<string> opts;
    const char* expected_name;
};


ostream& operator<<(ostream& os, const SubvolCase& c)
{
    return os << c.label;
}


const SubvolCase subvol_cases[] = {
    { "named_subvol",         { "rw", "relatime", "subvol=@root", "subvolid=256" },     "@root" },
    { "root_subvol",          { "rw", "relatime", "subvol=/", "subvolid=5" },           "" },
    { "no_subvol_option",     { "rw", "relatime", "subvolid=256" },                     "" },
    { "at_sign_only",         { "rw", "subvol=@" },                                     "@" },
    { "nested_subvol",        { "rw", "subvol=root/@root" },                            "root/@root" },
    { "empty_options",        { },                                                      "" },
    { "leading_slash_named",  { "rw", "relatime", "subvol=/@rootfs", "subvolid=256" },  "@rootfs" },
    { "leading_slash_at",     { "rw", "relatime", "subvol=/@root", "subvolid=256" },    "@root" },
    { "plain_name_no_at",     { "rw", "relatime", "subvol=root", "subvolid=256" },      "root" },
    { "leading_slash_plain",  { "rw", "relatime", "subvol=/root", "subvolid=256" },     "root" },
    { "tumbleweed_nested",    { "rw", "subvol=/@/.snapshots/1/snapshot" },              "@/.snapshots/1/snapshot" },
};


BOOST_DATA_TEST_CASE(subvol_name, boost::unit_test::data::make(subvol_cases), c)
{
    BOOST_CHECK_EQUAL(subvol_name_from_options(c.opts), string(c.expected_name));
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
