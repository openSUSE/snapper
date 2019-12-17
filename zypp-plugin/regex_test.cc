#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE regex

#include <boost/test/unit_test.hpp>
#include "solvable_matcher.h"

static
SolvableMatcher rx(const char* regex) {
    bool is_important = false;
    return SolvableMatcher(regex, SolvableMatcher::Kind::REGEX, is_important);
}

BOOST_AUTO_TEST_CASE(regex)
{
    BOOST_CHECK(rx("mypkg").match("mypkg"));
    BOOST_CHECK(!rx("mypkg").match("yourpkg"));

    // substrings don't count
    BOOST_CHECK(!rx("foo").match("afool"));
    // prefixes must match
    BOOST_CHECK(rx("foo").match("fool"));
    // explicit anchor is OK
    BOOST_CHECK(rx("^foo").match("foo"));
    // double anchor is also OK
    BOOST_CHECK(rx("^^foo").match("foo"));
    // exclude prefix matches
    BOOST_CHECK(!rx("foo$").match("foo-devel"));

    SolvableMatcher ror = rx("ruby[.0-9]+-rubygem-active(model|record)");
    BOOST_CHECK(ror.match("ruby2.5-rubygem-activemodel-5.2"));
    BOOST_CHECK(ror.match("ruby2.5-rubygem-activerecord-5_1"));
}
