
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include "../Stomp.h"


using namespace std;
using namespace Stomp;


const string null("\0", 1);


BOOST_AUTO_TEST_CASE(test1)
{
    // no optional content-lenght

    istringstream s1("HELLO\nkey:value\n\nWORLD" + null);
    istream s2(s1.rdbuf());

    Message msg = read_message(s2);

    BOOST_CHECK_EQUAL(s2.peek(), -1);

    BOOST_CHECK_EQUAL(msg.command, "HELLO");

    BOOST_CHECK_EQUAL(msg.headers.size(), 1);
    BOOST_CHECK_EQUAL(msg.headers["key"], "value");

    BOOST_CHECK_EQUAL(msg.body, "WORLD");
}


BOOST_AUTO_TEST_CASE(test2)
{
    // optional content-lenght and body with null character

    istringstream s1("HELLO\nkey:value\ncontent-length:5\n\nW" + null + "RLD" + null);
    istream s2(s1.rdbuf());

    Message msg = read_message(s2);

    BOOST_CHECK_EQUAL(s2.peek(), -1);

    BOOST_CHECK_EQUAL(msg.command, "HELLO");

    BOOST_CHECK_EQUAL(msg.headers.size(), 2);
    BOOST_CHECK_EQUAL(msg.headers["key"], "value");
    BOOST_CHECK_EQUAL(msg.headers["content-length"], "5");

    BOOST_CHECK_EQUAL(msg.body, "W" + null + "RLD");
}


BOOST_AUTO_TEST_CASE(escape1)
{
    // special characters in header

    istringstream s1("HELLO\nGermany\\cSpain:2\\c1\n\nWORLD" + null);
    istream s2(s1.rdbuf());

    Message msg = read_message(s2);

    BOOST_CHECK_EQUAL(msg.command, "HELLO");

    BOOST_CHECK_EQUAL(msg.headers.size(), 1);
    BOOST_CHECK_EQUAL(msg.headers["Germany:Spain"], "2:1");

    BOOST_CHECK_EQUAL(msg.body, "WORLD");
}
