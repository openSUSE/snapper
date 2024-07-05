
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include "../Stomp.h"


using namespace std;
using namespace Stomp;


BOOST_AUTO_TEST_CASE(escape)
{
    BOOST_CHECK_EQUAL(Stomp::escape_header("hello"), "hello");

    BOOST_CHECK_EQUAL(Stomp::escape_header("hello\nworld"), "hello\\nworld");
    BOOST_CHECK_EQUAL(Stomp::escape_header("hello:world"), "hello\\cworld");
}


BOOST_AUTO_TEST_CASE(unescape)
{
    BOOST_CHECK_EQUAL(Stomp::unescape_header("hello"), "hello");

    BOOST_CHECK_EQUAL(Stomp::unescape_header("hello\\nworld"), "hello\nworld");
    BOOST_CHECK_EQUAL(Stomp::unescape_header("hello\\cworld"), "hello:world");
}
