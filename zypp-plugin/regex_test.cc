#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE regex

#include <boost/test/unit_test.hpp>
#include "solvable_matcher.h"

static
SolvableMatcher matcher(const char* regex) {
    bool is_important = false;
    return SolvableMatcher(regex, SolvableMatcher::Kind::REGEX, is_important);
}

BOOST_AUTO_TEST_CASE(regex)
{
    BOOST_CHECK(matcher("mypkg").match("mypkg"));
    BOOST_CHECK(!matcher("mypkg").match("yourpkg"));
}
