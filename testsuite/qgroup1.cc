
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE qgroup1

#include <boost/test/unit_test.hpp>

#include <snapper/BtrfsUtils.h>

using namespace snapper;
using namespace BtrfsUtils;


BOOST_AUTO_TEST_CASE(parse)
{
    BOOST_CHECK_EQUAL(parse_qgroup("0/0"), 0);
    BOOST_CHECK_EQUAL(parse_qgroup("0/2"), 2);
    BOOST_CHECK_EQUAL(parse_qgroup("1/0"), 1LLU << 48);
    BOOST_CHECK_EQUAL(parse_qgroup("1/2"), (1LLU << 48) + 2);
}


BOOST_AUTO_TEST_CASE(format)
{
    BOOST_CHECK_EQUAL(format_qgroup(0), "0/0");
    BOOST_CHECK_EQUAL(format_qgroup(2), "0/2");
    BOOST_CHECK_EQUAL(format_qgroup(1LLU << 48), "1/0");
    BOOST_CHECK_EQUAL(format_qgroup((1LLU << 48) + 2), "1/2");
}
