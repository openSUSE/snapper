
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include "../Stomp.h"


using namespace std;
using namespace Stomp;


const string null("\0", 1);


BOOST_AUTO_TEST_CASE(test1)
{
    Message msg;
    msg.command = "HELLO";
    msg.headers["key"] = "value";
    msg.body = "WORLD";

    ostringstream s1;
    write_message(s1, msg);
    string s2 = s1.str();

    BOOST_CHECK_EQUAL(s2, "HELLO\nkey:value\n\nWORLD" + null);
}


BOOST_AUTO_TEST_CASE(escape1)
{
    // special characters in header

    Message msg;
    msg.command = "HELLO";
    msg.headers["Germany:Spain"] = "2:1";
    msg.body = "WORLD";

    ostringstream s1;
    write_message(s1, msg);
    string s2 = s1.str();

    BOOST_CHECK_EQUAL(s2, "HELLO\nGermany\\cSpain:2\\c1\n\nWORLD" + null);
}
