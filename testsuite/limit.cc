
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include <locale>

#include <snapper/Exception.h>
#include "../client/utils/Limit.h"
#include "../client/utils/HumanString.h"


using namespace std;
using namespace snapper;


string
test(const char* loc, const string& s)
{
    locale::global(locale(loc));

    Limit limit(0.5);
    limit.parse(s);

    locale::global(locale::classic());
    
    ostringstream tmp;
    tmp << limit;
    return tmp.str();
}


BOOST_AUTO_TEST_CASE(parse1)
{
    BOOST_CHECK_EQUAL(test("en_US.UTF-8", "0.10"), "0.1");
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", "0.10"), "0.1");
    BOOST_CHECK_EQUAL(test("fr_FR.UTF-8", "0.10"), "0.1");
}


BOOST_AUTO_TEST_CASE(parse2)
{
    BOOST_CHECK_EQUAL(test("en_US.UTF-8", "2.5 GiB"), "2.50 GiB");
    BOOST_CHECK_EQUAL(test("de_DE.UTF-8", "2.5 GiB"), "2.50 GiB");
    BOOST_CHECK_EQUAL(test("fr_FR.UTF-8", "2.5 GiB"), "2.50 GiB");

    BOOST_CHECK_EQUAL(test("en_US.UTF-8", "2.5 gib"), "2.50 GiB");
    BOOST_CHECK_EQUAL(test("en_US.UTF-8", "2.5 gb"), "2.50 GiB");
}


BOOST_AUTO_TEST_CASE(error1)
{
    BOOST_CHECK_THROW(test("de_DE.UTF-8", "0,5"), Exception);
}


BOOST_AUTO_TEST_CASE(error2)
{
    BOOST_CHECK_THROW(test("de_DE.UTF-8", "2,5 GiB"), Exception);
    BOOST_CHECK_THROW(test("fr_FR.UTF-8", "2,5 Gio"), Exception);
}


BOOST_AUTO_TEST_CASE(test1)
{
    MaxUsedLimit used_limit(0.5);

    BOOST_CHECK(used_limit.is_enabled());

    BOOST_CHECK(used_limit.is_satisfied(1 * TiB, 0.45 * TiB));
    BOOST_CHECK(!used_limit.is_satisfied(1 * TiB, 0.55 * TiB));
}


BOOST_AUTO_TEST_CASE(test2)
{
    MinFreeLimit free_limit(0.2);

    BOOST_CHECK(free_limit.is_enabled());

    BOOST_CHECK(free_limit.is_satisfied(1 * TiB, 0.25 * TiB));
    BOOST_CHECK(!free_limit.is_satisfied(1 * TiB, 0.15 * TiB));
}
