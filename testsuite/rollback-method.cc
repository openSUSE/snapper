
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE rollback_method

#include <boost/test/unit_test.hpp>

#include "config.h"
#include <snapper/AppUtil.h>

using namespace std;
using namespace snapper;


BOOST_AUTO_TEST_CASE(named_subvol)
{
    string name;
    vector<string> opts = { "rw", "relatime", "subvol=@root", "subvolid=256" };
    BOOST_CHECK(detect_rollback_method_from_options(opts, name) == RollbackMethod::SUBVOL_RENAME);
    BOOST_CHECK_EQUAL(name, "@root");
}


BOOST_AUTO_TEST_CASE(root_subvol)
{
    string name;
    vector<string> opts = { "rw", "relatime", "subvol=/", "subvolid=5" };
    BOOST_CHECK(detect_rollback_method_from_options(opts, name) == RollbackMethod::SET_DEFAULT);
    BOOST_CHECK(name.empty());
}


BOOST_AUTO_TEST_CASE(no_subvol_option)
{
    // only subvolid=, no subvol= — default subvolume mount
    string name;
    vector<string> opts = { "rw", "relatime", "subvolid=256" };
    BOOST_CHECK(detect_rollback_method_from_options(opts, name) == RollbackMethod::SET_DEFAULT);
    BOOST_CHECK(name.empty());
}


BOOST_AUTO_TEST_CASE(at_sign_only)
{
    string name;
    vector<string> opts = { "rw", "subvol=@" };
    BOOST_CHECK(detect_rollback_method_from_options(opts, name) == RollbackMethod::SUBVOL_RENAME);
    BOOST_CHECK_EQUAL(name, "@");
}


BOOST_AUTO_TEST_CASE(nested_subvol)
{
    string name;
    vector<string> opts = { "rw", "subvol=root/@root" };
    BOOST_CHECK(detect_rollback_method_from_options(opts, name) == RollbackMethod::SUBVOL_RENAME);
    BOOST_CHECK_EQUAL(name, "root/@root");
}


BOOST_AUTO_TEST_CASE(empty_options)
{
    string name;
    vector<string> opts = {};
    BOOST_CHECK(detect_rollback_method_from_options(opts, name) == RollbackMethod::SET_DEFAULT);
    BOOST_CHECK(name.empty());
}


BOOST_AUTO_TEST_CASE(subvolid_not_matched_as_subvol)
{
    // subvolid= must not be mistaken for subvol=
    string name;
    vector<string> opts = { "rw", "subvolid=5" };
    BOOST_CHECK(detect_rollback_method_from_options(opts, name) == RollbackMethod::SET_DEFAULT);
    BOOST_CHECK(name.empty());
}
