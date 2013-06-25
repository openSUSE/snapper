
#include <boost/algorithm/string.hpp>

#include "common.h"

#include <snapper/AsciiFile.h>

using namespace snapper;


int
main()
{
    SysconfigFile s("sysconfig-get1.txt");

    string tmp_string;

    check_true(s.getValue("S1", tmp_string));
    check_equal(tmp_string, string("hello"));

    bool tmp_bool;

    check_true(s.getValue("B1", tmp_bool));
    check_equal(tmp_bool, true);

    check_true(s.getValue("B2", tmp_bool));
    check_equal(tmp_bool, false);

    vector<string> tmp_vector;

    check_true(s.getValue("V1", tmp_vector));
    check_equal(boost::join(tmp_vector, "-"), string("one word"));

    check_true(s.getValue("V2", tmp_vector));
    check_equal(boost::join(tmp_vector, "-"), string("two-words"));

    check_true(s.getValue("V3", tmp_vector));
    check_equal(boost::join(tmp_vector, "-"), string("now-three-words"));

    check_true(s.getValue("V4", tmp_vector));
    check_equal(boost::join(tmp_vector, "-"), string("c:\\io.sys"));

    check_true(!s.getValue("V5", tmp_vector));

    exit(EXIT_SUCCESS);
}
