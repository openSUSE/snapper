
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include "../client/utils/CsvFormatter.h"


using namespace std;


BOOST_AUTO_TEST_CASE(test1)
{
    vector<string> columns = { "column1", "column2", "column3" };

    vector<vector<string>> rows = {
	{ "value;1", "value\n2", "value\"3" },
	{ "value1", "\"value2\"", "value3" }
    };

    string separator = ";";

    snapper::cli::CsvFormatter formatter(columns, rows, separator);

    string result =
	"column1;column2;column3\n"
	"\"value;1\";\"value\n2\";\"value\"\"3\"\n"
	"value1;\"\"\"value2\"\"\";value3\n";

    BOOST_CHECK_EQUAL(formatter.str(), result);
}
