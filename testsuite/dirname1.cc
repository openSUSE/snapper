
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE dirname1

#include <boost/test/unit_test.hpp>

#include <snapper/AppUtil.h>

using namespace snapper;


BOOST_AUTO_TEST_CASE(dirname1)
{
    BOOST_CHECK_EQUAL(dirname("/hello/world"), "/hello");
    BOOST_CHECK_EQUAL(dirname("hello/world"), "hello");
    BOOST_CHECK_EQUAL(dirname("/hello"), "/");
    BOOST_CHECK_EQUAL(dirname("hello"), ".");
    BOOST_CHECK_EQUAL(dirname("/"), "/");
    BOOST_CHECK_EQUAL(dirname(""), ".");
    BOOST_CHECK_EQUAL(dirname("."), ".");
    BOOST_CHECK_EQUAL(dirname(".."), ".");
    BOOST_CHECK_EQUAL(dirname("../.."), "..");
}
