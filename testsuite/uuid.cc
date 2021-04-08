
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <sstream>
#include <boost/test/unit_test.hpp>

#include "../snapper/AppUtil.h"

using namespace snapper;


BOOST_AUTO_TEST_CASE(test1)
{
    uint8_t value[16] = { 0x78, 0x9e, 0x39, 0xcd, 0x96, 0x7f, 0x41, 0xf0,
			  0x8a, 0x17, 0xcc, 0xc8, 0xf0, 0x00, 0x2e, 0xae };

    Uuid uuid;
    std::copy(std::begin(value), std::end(value), std::begin(uuid.value));
    std::ostringstream s;
    s << uuid;

    BOOST_CHECK_EQUAL(s.str(), "789e39cd-967f-41f0-8a17-ccc8f0002eae");
}
