
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include "../client/utils/equal-date.h"


bool
equal_week(const char* s1, const char* s2)
{
    struct tm tmp1;
    memset(&tmp1, 0, sizeof(tmp1));
    strptime(s1, "%Y-%m-%d", &tmp1);

    struct tm tmp2;
    memset(&tmp2, 0, sizeof(tmp2));
    strptime(s2, "%Y-%m-%d", &tmp2);

    return equal_week(tmp1, tmp2);
}


BOOST_AUTO_TEST_CASE(test1)
{
    // 2012 is a leap year
    BOOST_CHECK(equal_week("2011-12-31", "2012-01-01"));
    BOOST_CHECK(equal_week("2012-01-01", "2011-12-31"));
}


BOOST_AUTO_TEST_CASE(test2)
{
    // 2012 is a leap year
    BOOST_CHECK(equal_week("2012-12-31", "2013-01-01"));
    BOOST_CHECK(equal_week("2013-01-01", "2012-12-31"));
}


BOOST_AUTO_TEST_CASE(test3)
{
    // Saturday and Sunday
    BOOST_CHECK(equal_week("2014-01-04", "2014-01-05"));
    BOOST_CHECK(equal_week("2014-01-05", "2014-01-04"));

    // Sunday and Monday
    BOOST_CHECK(!equal_week("2014-01-05", "2014-01-06"));
    BOOST_CHECK(!equal_week("2014-01-06", "2014-01-05"));

    // Monday and Tuesday
    BOOST_CHECK(equal_week("2014-01-06", "2014-01-07"));
    BOOST_CHECK(equal_week("2014-01-07", "2014-01-06"));
}


BOOST_AUTO_TEST_CASE(test4)
{
    // 2014-12-31 is a Wednesday, 2015-01-01 is a Thursday
    BOOST_CHECK(equal_week("2014-12-31", "2015-01-01"));
    BOOST_CHECK(equal_week("2015-01-01", "2014-12-31"));
}


BOOST_AUTO_TEST_CASE(test5)
{
    // 2017-12-31 is a Sunday, 2018-01-01 is a Monday
    BOOST_CHECK(!equal_week("2017-12-31", "2018-01-01"));
    BOOST_CHECK(!equal_week("2018-01-01", "2017-12-31"));
}
