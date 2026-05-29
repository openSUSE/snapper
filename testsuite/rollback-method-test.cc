#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE rollback_method

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include "config.h"
#include "client/snapper/rollback-method.h"

using namespace std;
using namespace snapper;


struct SubvolCase
{
    const char* label;
    vector<string> opts;
    RollbackMethod expected_method;
};


ostream& operator<<(ostream& os, const SubvolCase& c)
{
    return os << c.label;
}


const SubvolCase cases[] = {
    { "named_subvol",              { "rw", "relatime", "subvol=@root", "subvolid=256" },    RollbackMethod::SUBVOL_RENAME },
    { "root_subvol",               { "rw", "relatime", "subvol=/", "subvolid=5" },          RollbackMethod::SET_DEFAULT },
    { "no_subvol_option",          { "rw", "relatime", "subvolid=256" },                    RollbackMethod::SET_DEFAULT },
    { "at_sign_only",              { "rw", "subvol=@" },                                    RollbackMethod::SUBVOL_RENAME },
    { "nested_subvol",             { "rw", "subvol=root/@root" },                           RollbackMethod::SET_DEFAULT },
    { "empty_options",             { },                                                     RollbackMethod::SET_DEFAULT },
    { "subvolid_not_matched",      { "rw", "subvolid=5" },                                  RollbackMethod::SET_DEFAULT },
    { "leading_slash_named",       { "rw", "relatime", "subvol=/@rootfs", "subvolid=256" }, RollbackMethod::SUBVOL_RENAME },
    { "leading_slash_at",          { "rw", "relatime", "subvol=/@root", "subvolid=256" },   RollbackMethod::SUBVOL_RENAME },
    { "plain_name_no_at",          { "rw", "relatime", "subvol=root", "subvolid=256" },     RollbackMethod::SUBVOL_RENAME },
    { "leading_slash_plain",       { "rw", "relatime", "subvol=/root", "subvolid=256" },    RollbackMethod::SUBVOL_RENAME },
    { "tumbleweed_nested",         { "rw", "subvol=/@/.snapshots/1/snapshot" },              RollbackMethod::SET_DEFAULT },
};


BOOST_DATA_TEST_CASE(detect_method, boost::unit_test::data::make(cases), c)
{
    BOOST_CHECK(detect_rollback_method_from_options(c.opts) == c.expected_method);
}


struct AmbitCase
{
    const char* label;
    RollbackMethod method;
    SubvolumeMode default_state;
    Ambit expected;
};


ostream& operator<<(ostream& os, const AmbitCase& c)
{
    return os << c.label;
}


const AmbitCase ambit_cases[] = {
    { "subvol_rename_unknown_classic",        RollbackMethod::SUBVOL_RENAME, SubvolumeMode::UNKNOWN,    Ambit::CLASSIC },
    { "subvol_rename_ro_classic",             RollbackMethod::SUBVOL_RENAME, SubvolumeMode::READ_ONLY,  Ambit::CLASSIC },
    { "subvol_rename_rw_classic",             RollbackMethod::SUBVOL_RENAME, SubvolumeMode::READ_WRITE, Ambit::CLASSIC },
    { "set_default_unknown_auto",             RollbackMethod::SET_DEFAULT,   SubvolumeMode::UNKNOWN,    Ambit::AUTO },
    { "set_default_readonly_transactional",   RollbackMethod::SET_DEFAULT,   SubvolumeMode::READ_ONLY,  Ambit::TRANSACTIONAL },
    { "set_default_readwrite_classic",        RollbackMethod::SET_DEFAULT,   SubvolumeMode::READ_WRITE, Ambit::CLASSIC },
};


BOOST_DATA_TEST_CASE(detect_ambit_test, boost::unit_test::data::make(ambit_cases), c)
{
    BOOST_CHECK(detect_ambit(c.method, c.default_state) == c.expected);
}
