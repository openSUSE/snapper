#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE rollback_method

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include "config.h"
#include "client/snapper/rollback-method.h"

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


// --- mechanism: use_subvol_rename ---------------------------------------------

struct MechanismCase
{
    const char* label;
    const char* rollback_method;	// value of ROLLBACK_METHOD
    const char* subvol_name;		// named root subvolume ("" = default subvol id)
    bool expected;
};


ostream& operator<<(ostream& os, const MechanismCase& c)
{
    return os << c.label;
}


const MechanismCase mechanism_cases[] = {
    // auto (and the empty default) select rename only for a top-level named subvolume
    { "auto_named",        "auto", "@root",      true },
    { "empty_named",       "",     "@root",      true },
    { "auto_default",      "auto", "",           false },
    { "auto_nested",       "auto", "root/@root", false },

    // set-default forces the default-subvol-id mechanism even on a named mount
    { "setdefault_named",  "set-default", "@root", false },
    { "setdefault_default","set-default", "",      false },

    // subvol-rename forces rename on a top-level named subvolume
    { "rename_named",      "subvol-rename", "@root", true },
};


BOOST_DATA_TEST_CASE(mechanism, boost::unit_test::data::make(mechanism_cases), c)
{
    BOOST_CHECK_EQUAL(use_subvol_rename(c.rollback_method, c.subvol_name), c.expected);
}


// subvol-rename requested where it cannot work must be rejected
BOOST_AUTO_TEST_CASE(mechanism_rejects_rename_without_named_subvolume)
{
    BOOST_CHECK_THROW(use_subvol_rename("subvol-rename", ""), Exception);
    BOOST_CHECK_THROW(use_subvol_rename("subvol-rename", "root/@root"), Exception);
}


// an unknown ROLLBACK_METHOD value must be rejected
BOOST_AUTO_TEST_CASE(mechanism_rejects_unknown_method)
{
    BOOST_CHECK_THROW(use_subvol_rename("bogus", ""), Exception);
}


// --- semantic: classic_or_transactional ---------------------------------------

struct SemanticCase
{
    const char* label;
    Ambit cli_ambit;			// --ambit (AUTO if not given)
    SubvolumeMode mode;			// read-only/-write state of default snapshot
    Ambit expected;
};


ostream& operator<<(ostream& os, const SemanticCase& c)
{
    return os << c.label;
}


const SemanticCase semantic_cases[] = {
    // auto: derive from the read-only/-write state of the default snapshot
    { "auto_rw",           Ambit::AUTO,          SubvolumeMode::READ_WRITE, Ambit::CLASSIC },
    { "auto_ro",           Ambit::AUTO,          SubvolumeMode::READ_ONLY,  Ambit::TRANSACTIONAL },
    { "auto_unknown",      Ambit::AUTO,          SubvolumeMode::UNKNOWN,    Ambit::AUTO },

    // explicit --ambit overrides mode detection
    { "cli_classic",       Ambit::CLASSIC,       SubvolumeMode::UNKNOWN,    Ambit::CLASSIC },
    { "cli_transactional", Ambit::TRANSACTIONAL, SubvolumeMode::READ_WRITE, Ambit::TRANSACTIONAL },
    { "cli_over_ro",       Ambit::CLASSIC,       SubvolumeMode::READ_ONLY,  Ambit::CLASSIC },
};


BOOST_DATA_TEST_CASE(semantic, boost::unit_test::data::make(semantic_cases), c)
{
    BOOST_CHECK(classic_or_transactional(c.cli_ambit, c.mode) == c.expected);
}
