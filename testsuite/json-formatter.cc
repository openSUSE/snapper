
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include <vector>
#include <string>

#include "../client/utils/JsonFormatter.h"

using namespace std;

BOOST_AUTO_TEST_CASE(test1_escape_values)
{
    snapper::cli::JsonFormatter::Data data = {
	{ "key1", "value1" },
	{ "key2", "value\"2" },
	{ "key3", "value\\3" },
	{ "key3", "value\b4" },
	{ "key3", "value\f5" },
	{ "key3", "value\n6" },
	{ "key3", "value\r7" },
	{ "key3", "value\t8" },
	{ "key4", "\"value9\"" }
    };

    string expected_result =
	"{\n"
	"  \"key1\": \"value1\",\n"
	"  \"key2\": \"value\\\"2\",\n"
	"  \"key3\": \"value\\\\3\",\n"
	"  \"key3\": \"value\\b4\",\n"
	"  \"key3\": \"value\\f5\",\n"
	"  \"key3\": \"value\\n6\",\n"
	"  \"key3\": \"value\\r7\",\n"
	"  \"key3\": \"value\\t8\",\n"
	"  \"key4\": \"\\\"value9\\\"\"\n"
	"}";

    snapper::cli::JsonFormatter formatter(data);

    BOOST_CHECK_EQUAL(formatter.output(), expected_result);
}


BOOST_AUTO_TEST_CASE(test2_skip_format)
{
    snapper::cli::JsonFormatter::Data data = {
	{ "key1", "true" },
	{ "key2", "value2" }
    };

    string expected_result =
	"{\n"
	"  \"key1\": true,\n"
	"  \"key2\": \"value2\"\n"
	"}";

    snapper::cli::JsonFormatter formatter(data);

    formatter.skip_format_values({ "key1" });

    BOOST_CHECK_EQUAL(formatter.output(), expected_result);
}


BOOST_AUTO_TEST_CASE(test3_indent)
{
    // Using an ident level

    snapper::cli::JsonFormatter::Data data = {
	{ "key1", "value1" },
	{ "key2", "value2" }
    };

    string expected_result =
	"  {\n"
	"    \"key1\": \"value1\",\n"
	"    \"key2\": \"value2\"\n"
	"  }";

    snapper::cli::JsonFormatter formatter(data);

    BOOST_CHECK_EQUAL(formatter.output(1), expected_result);
}


BOOST_AUTO_TEST_CASE(test4_inline)
{
    // Using inline

    snapper::cli::JsonFormatter::Data data = {
	{ "key1", "value1" },
	{ "key2", "value2" }
    };

    string expected_result =
	"{\n"
	"    \"key1\": \"value1\",\n"
	"    \"key2\": \"value2\"\n"
	"  }";

    snapper::cli::JsonFormatter formatter(data);

    formatter.set_inline(true);

    BOOST_CHECK_EQUAL(formatter.output(1), expected_result);
}


BOOST_AUTO_TEST_CASE(test5_list)
{
    vector<string> data = { "value1", "value2", "value3" };

    snapper::cli::JsonFormatter::List formatter(data);

    string result =
	"[\n"
	"value1,\n"
	"value2,\n"
	"value3\n"
	"]";

    BOOST_CHECK_EQUAL(formatter.output(), result);
}
