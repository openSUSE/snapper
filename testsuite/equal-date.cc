
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include "../client/utils/equal-date.h"
#include "../snapper/AppUtil.h"

using namespace snapper;


bool
equal_week(const char* s1, const char* s2)
{
    // use interim time_t since strptime on musl does not set tm_yday

    time_t t1 = scan_datetime(s1, true);
    struct tm tmp1;
    memset(&tmp1, 0, sizeof(tmp1));
    gmtime_r(&t1, &tmp1);

    time_t t2 = scan_datetime(s2, true);
    struct tm tmp2;
    memset(&tmp2, 0, sizeof(tmp2));
    gmtime_r(&t2, &tmp2);

    return equal_week(tmp1, tmp2);
}


BOOST_AUTO_TEST_CASE(test1)
{
    // 2012 is a leap year
    BOOST_CHECK(equal_week("2011-12-31 00:00:00", "2012-01-01 00:00:00"));
    BOOST_CHECK(equal_week("2012-01-01 00:00:00", "2011-12-31 00:00:00"));
}


BOOST_AUTO_TEST_CASE(test2)
{
    // 2012 is a leap year
    BOOST_CHECK(equal_week("2012-12-31 00:00:00", "2013-01-01 00:00:00"));
    BOOST_CHECK(equal_week("2013-01-01 00:00:00", "2012-12-31 00:00:00"));
}


BOOST_AUTO_TEST_CASE(test3)
{
    // Saturday and Sunday
    BOOST_CHECK(equal_week("2014-01-04 00:00:00", "2014-01-05 00:00:00"));
    BOOST_CHECK(equal_week("2014-01-05 00:00:00", "2014-01-04 00:00:00"));

    // Sunday and Monday
    BOOST_CHECK(!equal_week("2014-01-05 00:00:00", "2014-01-06 00:00:00"));
    BOOST_CHECK(!equal_week("2014-01-06 00:00:00", "2014-01-05 00:00:00"));

    // Monday and Tuesday
    BOOST_CHECK(equal_week("2014-01-06 00:00:00", "2014-01-07 00:00:00"));
    BOOST_CHECK(equal_week("2014-01-07 00:00:00", "2014-01-06 00:00:00"));
}


BOOST_AUTO_TEST_CASE(test4)
{
    // 2014-12-31 is a Wednesday, 2015-01-01 is a Thursday
    BOOST_CHECK(equal_week("2014-12-31 00:00:00", "2015-01-01 00:00:00"));
    BOOST_CHECK(equal_week("2015-01-01 00:00:00", "2014-12-31 00:00:00"));
}


BOOST_AUTO_TEST_CASE(test5)
{
    // 2017-12-31 is a Sunday, 2018-01-01 is a Monday
    BOOST_CHECK(!equal_week("2017-12-31 00:00:00", "2018-01-01 00:00:00"));
    BOOST_CHECK(!equal_week("2018-01-01 00:00:00", "2017-12-31 00:00:00"));
}
