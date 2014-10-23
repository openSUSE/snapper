
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE sysconfig_get1

#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string.hpp>

#include <snapper/AsciiFile.h>

using namespace snapper;


BOOST_AUTO_TEST_CASE(sysconfig_get1)
{
    SysconfigFile s("sysconfig-get1.txt");

    string tmp_string;

    BOOST_CHECK(s.getValue("S1", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "hello");

    bool tmp_bool;

    BOOST_CHECK(s.getValue("B1", tmp_bool));
    BOOST_CHECK_EQUAL(tmp_bool, true);

    BOOST_CHECK(s.getValue("B2", tmp_bool));
    BOOST_CHECK_EQUAL(tmp_bool, false);

    vector<string> tmp_vector;

    BOOST_CHECK(s.getValue("V1", tmp_vector));
    BOOST_CHECK_EQUAL(boost::join(tmp_vector, "-"), "one word");

    BOOST_CHECK(s.getValue("V2", tmp_vector));
    BOOST_CHECK_EQUAL(boost::join(tmp_vector, "-"), "two-words");

    BOOST_CHECK(s.getValue("V3", tmp_vector));
    BOOST_CHECK_EQUAL(boost::join(tmp_vector, "-"), "now-three-words");

    BOOST_CHECK(s.getValue("V4", tmp_vector));
    BOOST_CHECK_EQUAL(boost::join(tmp_vector, "-"), "c:\\io.sys");

    BOOST_CHECK(!s.getValue("V5", tmp_vector));
}
