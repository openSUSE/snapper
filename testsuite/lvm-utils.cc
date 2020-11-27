
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE basename1

#include <boost/test/unit_test.hpp>

#include <snapper/LvmUtils.h>

using namespace snapper;


BOOST_AUTO_TEST_CASE(split_device_name)
{
    std::pair<string, string> n1 = LvmUtils::split_device_name("/dev/mapper/system-root");
    BOOST_CHECK_EQUAL(n1.first, "system");
    BOOST_CHECK_EQUAL(n1.second, "root");

    std::pair<string, string> n2 = LvmUtils::split_device_name("/dev/mapper/vg--system-lv--root");
    BOOST_CHECK_EQUAL(n2.first, "vg-system");
    BOOST_CHECK_EQUAL(n2.second, "lv-root");

    std::pair<string, string> n3 = LvmUtils::split_device_name("/dev/mapper/s-r");
    BOOST_CHECK_EQUAL(n3.first, "s");
    BOOST_CHECK_EQUAL(n3.second, "r");
}
