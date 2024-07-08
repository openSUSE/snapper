
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include "../Stomp.h"


using namespace std;
using namespace Stomp;


BOOST_AUTO_TEST_CASE(cr)
{
    BOOST_CHECK_EQUAL(Stomp::strip_cr("hello"), "hello");
    BOOST_CHECK_EQUAL(Stomp::strip_cr("hello\r"), "hello");
    BOOST_CHECK_EQUAL(Stomp::strip_cr("hello\r\n"), "hello\r\n");
}
