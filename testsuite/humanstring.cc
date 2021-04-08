
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include <locale>

#include <snapper/Exception.h>
#include "../client/utils/HumanString.h"


using namespace std;
using namespace snapper;


// Tests here must work when using double instead of long double in
// HumanString.cc.


string
test(const char* loc, unsigned long long size, int precision)
{
    locale::global(locale(loc));

    return byte_to_humanstring(size, false, precision);
}


unsigned long long
test(const char* loc, const char* str)
{
    locale::global(locale(loc));

    return humanstring_to_byte(str, false);
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

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 50 * MiB, 2), "50.00 MiB");
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", 50 * MiB, 2), "50,00 MiB");
    BOOST_CHECK_EQUAL(test("de_CH.UTF-8", 50 * MiB, 2), "50.00 MiB");
}


BOOST_AUTO_TEST_CASE(test_humanstring_to_byte)
{
    BOOST_CHECK_THROW(test("en_GB.UTF-8", "hello"), Exception);

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "0 B"), 0);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "-0 B"), 0);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "+0 B"), 0);

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "42B"), 42);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "42 b"), 42);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "42"), 42);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "42b"), 42);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "42 B"), 42);

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "12.4GB"), 13314398618);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "12.4 GB"), 13314398618);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "12.4 gb"), 13314398618);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "12.4g"), 13314398618);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "12.4 G"), 13314398618);

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "123,456 kB"), 126418944);
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", "123.456 kB"), 126418944);
    BOOST_CHECK_EQUAL(test("de_CH.UTF-8", "123'456 kB"), 126418944);

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "123,456.789kB"), 126419752);
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", "123.456,789kB"), 126419752);
    BOOST_CHECK_EQUAL(test("de_CH.UTF-8", "123'456.789kB"), 126419752);

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "123,456.789 kB"), 126419752);
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", "123.456,789 kB"), 126419752);
    BOOST_CHECK_EQUAL(test("de_CH.UTF-8", "123'456.789 kB"), 126419752);

    BOOST_CHECK_THROW(test("en_US.UTF-8", "5 G B"), Exception);

    BOOST_CHECK_THROW(test("de_DE.UTF-8", "12.34 kB"), Exception);
    BOOST_CHECK_THROW(test("de_DE.UTF-8", "12'34 kB"), Exception);

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "3.14 G"), 3371549327);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "3.14 GB"), 3371549327);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "3.14 GiB"), 3371549327);

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "12345 GB"), 13255342817280);
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", "12345 GB"), 13255342817280);
    BOOST_CHECK_EQUAL(test("de_CH.UTF-8", "12345 GB"), 13255342817280);

    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", ".5 GiB"), 512 * MiB);
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", ",5 GiB"), 512 * MiB);
}


BOOST_AUTO_TEST_CASE(test_big_numbers)
{
    // 1 EiB - 1 B
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 1 * EiB - 1 * B, 2), "1,024.00 PiB");

    // 1 EiB
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 1 * EiB, 2), "1.00 EiB");
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "1 EiB"), 1 * EiB);
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "1.00 EiB"), 1 * EiB);

    // 1 EiB + 1 B
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 1 * EiB + 1 * B, 2), "1.00 EiB");

    // 16 EiB - 1 B
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", 16 * EiB - 1 * B, 2), "16.00 EiB");
    BOOST_CHECK_EQUAL(test("en_GB.UTF-8", "18446744073709551615 B"), 16 * EiB - 1 * B);

    // 16 EiB
    BOOST_CHECK_THROW(test("en_GB.UTF-8", "16 EiB"), Exception);
    BOOST_CHECK_THROW(test("en_GB.UTF-8", "18446744073709551616 B"), Exception);
}


BOOST_AUTO_TEST_CASE(test_ridiculous_high_numbers)
{
    // The unshifted value fits 64-bit IEEE but the shifted value
    // overflows. Tests error handling if long double is 64-bit IEEE.
    BOOST_CHECK_THROW(test("en_GB.UTF-8", "1.0E305 EiB"), Exception);

    // The unshifted value fits 80-bit IEEE but the shifted value
    // overflows. Tests error handling if long double is 80-bit IEEE.
    BOOST_CHECK_THROW(test("en_GB.UTF-8", "1.0E4930 EiB"), Exception);

    // Even the unshifted value is too high for 80-bit (and even 128-bit) IEEE.
    BOOST_CHECK_THROW(test("en_GB.UTF-8", "1.0E5000 B"), Exception);
}


BOOST_AUTO_TEST_CASE(test_negative_numbers)
{
    BOOST_CHECK_THROW(test("en_GB.UTF-8", "-1 B"), Exception);
    BOOST_CHECK_THROW(test("en_GB.UTF-8", "-1.0 B"), Exception);
}
