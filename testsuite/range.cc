
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include <locale>

#include <snapper/Exception.h>
#include "../client/utils/Range.h"

using namespace std;
using namespace snapper;


string
test(const char* loc, const string& s)
{
    locale::global(locale(loc));

    Range range(10);
    range.parse(s);

    ostringstream tmp;
    tmp << range;
    return tmp.str();
}


BOOST_AUTO_TEST_CASE(parse1)
{
    BOOST_CHECK_EQUAL(test("en_US.UTF-8", "10"), "10-10");
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", "10"), "10-10");
}


BOOST_AUTO_TEST_CASE(parse2)
{
    BOOST_CHECK_EQUAL(test("en_US.UTF-8", "10-20"), "10-20");
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", "10-20"), "10-20");
}


BOOST_AUTO_TEST_CASE(error1)
{
    BOOST_CHECK_THROW(test("en_US.UTF-8", "10 - 20"), Exception);
    BOOST_CHECK_THROW(test("en_US.UTF-8", "10-"), Exception);
    BOOST_CHECK_THROW(test("en_US.UTF-8", "-20"), Exception);

    BOOST_CHECK_THROW(test("en_US.UTF-8", "a-b"), Exception);
}


BOOST_AUTO_TEST_CASE(test1)
{
    Range range(10);

    BOOST_CHECK(range.is_degenerated());
}


BOOST_AUTO_TEST_CASE(test2)
{
    Range range(10);
    range.parse("2-10");

    BOOST_CHECK(!range.is_degenerated());
}
