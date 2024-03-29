
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

    BOOST_CHECK(s.get_value("S1", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "hello");

    BOOST_CHECK(s.get_value("S2", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "hello");

    bool tmp_bool;

    BOOST_CHECK(s.get_value("B1", tmp_bool));
    BOOST_CHECK_EQUAL(tmp_bool, true);

    BOOST_CHECK(s.get_value("B2", tmp_bool));
    BOOST_CHECK_EQUAL(tmp_bool, false);

    vector<string> tmp_vector;

    BOOST_CHECK(s.get_value("V1", tmp_vector));
    BOOST_CHECK_EQUAL(boost::join(tmp_vector, "-"), "one word");
    BOOST_CHECK_EQUAL(s.get_all_values()["V1"], "one\\ word");

    BOOST_CHECK(s.get_value("V2", tmp_vector));
    BOOST_CHECK_EQUAL(boost::join(tmp_vector, "-"), "two-words");

    BOOST_CHECK(s.get_value("V3", tmp_vector));
    BOOST_CHECK_EQUAL(boost::join(tmp_vector, "-"), "now-three-words");

    BOOST_CHECK(s.get_value("V4", tmp_vector));
    BOOST_CHECK_EQUAL(boost::join(tmp_vector, "-"), "c:\\io.sys");

    BOOST_CHECK(!s.get_value("V5", tmp_vector));

    BOOST_CHECK(s.get_value("V6", tmp_vector));
    BOOST_CHECK_EQUAL(boost::join(tmp_vector, "-"), "a-value-with-a-#-hash");
}


BOOST_AUTO_TEST_CASE(sysconfig_set1)
{
    system("cp sysconfig-set1.txt sysconfig-set1.txt.tmp");
    SysconfigFile s("sysconfig-set1.txt.tmp");

    string tmp_string;

    BOOST_CHECK(s.get_value("K2", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "changeme");

    s.set_value("K2", "all new");
    BOOST_CHECK(s.get_value("K2", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "all new");

    s.set_value("K2", "changeme");
    BOOST_CHECK(s.get_value("K2", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "changeme");

    s.save();
    BOOST_CHECK_EQUAL(system("diff -u sysconfig-set1.txt sysconfig-set1.txt.tmp"), 0);
}
