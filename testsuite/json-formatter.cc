
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string.hpp>

#include "../client/utils/JsonFormatter.h"


using namespace std;
using namespace snapper;


string
str(const JsonFormatter& formatter)
{
    ostringstream stream;
    stream << formatter;
    return stream.str();
}


string
trim(const string& s)
{
    list<string> tmp1;
    boost::split(tmp1, s, boost::is_any_of("\n"), boost::token_compress_on);

    for (string& tmp2 : tmp1)
	boost::trim(tmp2);

    return boost::algorithm::join(tmp1, "\n");
}


BOOST_AUTO_TEST_CASE(test1)
{
    JsonFormatter formatter;

    json_object_object_add(formatter.root(), "key1", json_object_new_string("hello"));
    json_object_object_add(formatter.root(), "key2", json_object_new_int(1000));
    json_object_object_add(formatter.root(), "key3", json_object_new_boolean(true));
    json_object_object_add(formatter.root(), "key4", nullptr);

    string expected_result =
	"{\n"
	"  \"key1\": \"hello\",\n"
	"  \"key2\": 1000,\n"
	"  \"key3\": true,\n"
	"  \"key4\": null\n"
	"}\n";

#if JSON_C_VERSION_NUM >= ((0 << 16) | (14 << 8) | 0)
    BOOST_CHECK_EQUAL(str(formatter), expected_result);
#else
    BOOST_CHECK_EQUAL(trim(str(formatter)), trim(expected_result));
#endif
}
