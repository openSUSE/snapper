#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE rollback_method

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include "config.h"
#include <snapper/AppUtil.h>

using namespace std;
using namespace snapper;


struct SubvolCase
{
    const char* label;
    vector<string> opts;
    RollbackMethod expected_method;
    string expected_name;
};


ostream& operator<<(ostream& os, const SubvolCase& c)
{
    return os << c.label;
}


const SubvolCase cases[] = {
    { "named_subvol",              { "rw", "relatime", "subvol=@root", "subvolid=256" },    RollbackMethod::SUBVOL_RENAME, "@root" },
    { "root_subvol",               { "rw", "relatime", "subvol=/", "subvolid=5" },          RollbackMethod::SET_DEFAULT,   "" },
    { "no_subvol_option",          { "rw", "relatime", "subvolid=256" },                    RollbackMethod::SET_DEFAULT,   "" },
    { "at_sign_only",              { "rw", "subvol=@" },                                    RollbackMethod::SUBVOL_RENAME, "@" },
    { "nested_subvol",             { "rw", "subvol=root/@root" },                           RollbackMethod::SUBVOL_RENAME, "root/@root" },
    { "empty_options",             { },                                                     RollbackMethod::SET_DEFAULT,   "" },
    { "subvolid_not_matched",      { "rw", "subvolid=5" },                                  RollbackMethod::SET_DEFAULT,   "" },
    { "leading_slash_named",       { "rw", "relatime", "subvol=/@rootfs", "subvolid=256" }, RollbackMethod::SUBVOL_RENAME, "@rootfs" },
    { "leading_slash_at",          { "rw", "relatime", "subvol=/@root", "subvolid=256" },   RollbackMethod::SUBVOL_RENAME, "@root" },
    { "plain_name_no_at",          { "rw", "relatime", "subvol=root", "subvolid=256" },     RollbackMethod::SUBVOL_RENAME, "root" },
    { "leading_slash_plain",       { "rw", "relatime", "subvol=/root", "subvolid=256" },    RollbackMethod::SUBVOL_RENAME, "root" },
};


BOOST_DATA_TEST_CASE(detect_method, boost::unit_test::data::make(cases), c)
{
    string name;
    BOOST_CHECK(detect_rollback_method_from_options(c.opts, name) == c.expected_method);
    BOOST_CHECK_EQUAL(name, c.expected_name);
}
