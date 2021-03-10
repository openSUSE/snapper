
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include "../snapper/AppUtil.h"

using namespace snapper;


BOOST_AUTO_TEST_CASE(test1)
{
    time_t t1 = scan_datetime("2020-03-04 12:34:56", true);

    struct tm tmp1;
    memset(&tmp1, 0, sizeof(tmp1));
    gmtime_r(&t1, &tmp1);

    BOOST_CHECK_EQUAL(tmp1.tm_year, 2020 - 1900);
    BOOST_CHECK_EQUAL(tmp1.tm_mon, 3 - 1);
    BOOST_CHECK_EQUAL(tmp1.tm_mday, 4);

    BOOST_CHECK_EQUAL(tmp1.tm_yday, 31 + 28 + 4);
    BOOST_CHECK_EQUAL(tmp1.tm_wday, 3);

    BOOST_CHECK_EQUAL(tmp1.tm_hour, 12);
    BOOST_CHECK_EQUAL(tmp1.tm_min, 34);
    BOOST_CHECK_EQUAL(tmp1.tm_sec, 56);
}
