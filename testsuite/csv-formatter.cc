
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include "../client/utils/CsvFormatter.h"


using namespace std;
using namespace snapper;


string
str(const CsvFormatter& formatter)
{
    ostringstream stream;
    stream << formatter;
    return stream.str();
}


BOOST_AUTO_TEST_CASE(test1)
{
    CsvFormatter formatter(";");

    formatter.header() = { "column1", "column2", "column3" };

    formatter.rows() = {
	{ "value;1", "value\n2", "value\"3" },
	{ "value1", "\"value2\"", "value3" }
    };

    string result =
	"column1;column2;column3\n"
	"\"value;1\";\"value\n2\";\"value\"\"3\"\n"
	"value1;\"\"\"value2\"\"\";value3\n";

    BOOST_CHECK_EQUAL(str(formatter), result);
}
