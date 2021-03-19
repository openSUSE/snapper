
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE root_prefix

#include <boost/test/unit_test.hpp>

#include <snapper/AppUtil.h>

using namespace snapper;


BOOST_AUTO_TEST_CASE(root_prefix)
{
    BOOST_CHECK_EQUAL(prepend_root_prefix("", "/"), "/");
    BOOST_CHECK_EQUAL(prepend_root_prefix("", "/home"), "/home");

    BOOST_CHECK_EQUAL(prepend_root_prefix("/", "/"), "/");
    BOOST_CHECK_EQUAL(prepend_root_prefix("/", "/home"), "/home");

    BOOST_CHECK_EQUAL(prepend_root_prefix("/mnt", "/"), "/mnt");
    BOOST_CHECK_EQUAL(prepend_root_prefix("/mnt", "/home"), "/mnt/home");
}
