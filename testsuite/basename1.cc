
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE basename1

#include <boost/test/unit_test.hpp>

#include <snapper/AppUtil.h>

using namespace snapper;


BOOST_AUTO_TEST_CASE(basename1)
{
    BOOST_CHECK_EQUAL(basename("/hello/world"), "world");
    BOOST_CHECK_EQUAL(basename("hello/world"), "world");
    BOOST_CHECK_EQUAL(basename("/hello"), "hello");
    BOOST_CHECK_EQUAL(basename("hello"), "hello");
    BOOST_CHECK_EQUAL(basename("/"), "");
    BOOST_CHECK_EQUAL(basename(""), "");
    BOOST_CHECK_EQUAL(basename("."), ".");
    BOOST_CHECK_EQUAL(basename(".."), "..");
    BOOST_CHECK_EQUAL(basename("../.."), "..");
}
