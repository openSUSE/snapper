
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include <stdlib.h>
#include <iostream>
#include <locale>

#include "../client/utils/HumanString.h"


using namespace std;
using namespace snapper;


// Tests here must work when using double instead of long double in
// HumanString.cc.


string
test(const char* loc, unsigned long long size, int precision)
{
    locale::global(locale(loc));

    return byte_to_humanstring(size, precision);
}



BOOST_AUTO_TEST_CASE(test_byte_to_humanstring)
{
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 0, 2), "0 B");

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 1023, 2), "1,023 B");

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 1024, 2), "1.00 KiB");

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 1025, 2), "1.00 KiB");

    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", 123456789, 4), "117,7376 MiB");

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 1000 * KiB, 2), "1,000.00 KiB");
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", 1000 * KiB, 2), "1.000,00 KiB");
    BOOST_CHECK_EQUAL(test("de_CH.UTF-8", 1000 * KiB, 2), "1'000.00 KiB");

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 50 * MiB, 2), "50.00 MiB");
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", 50 * MiB, 2), "50,00 MiB");
    BOOST_CHECK_EQUAL(test("de_CH.UTF-8", 50 * MiB, 2), "50.00 MiB");
}


BOOST_AUTO_TEST_CASE(test_big_numbers)
{
    // 1 EiB - 1 B
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 1 * EiB - 1 * B, 2), "1,024.00 PiB");

    // 1 EiB
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 1 * EiB, 2), "1.00 EiB");

    // 1 EiB + 1 B
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 1 * EiB + 1 * B, 2), "1.00 EiB");

    // 16 EiB - 1 B
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 16 * EiB - 1 * B, 2), "16.00 EiB");
}
