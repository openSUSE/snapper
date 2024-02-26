
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include "../client/utils/TableFormatter.h"


using namespace std;
using namespace snapper;


string
str(const TableFormatter& formatter)
{
    ostringstream stream;
    stream << formatter;
    return stream.str();
}


BOOST_AUTO_TEST_CASE(test1)
{
    locale::global(locale("en_GB.UTF-8"));

    TableFormatter formatter(Style::ASCII);

    formatter.header().push_back(Cell("Number", Id::NUMBER, Align::RIGHT));
    formatter.header().push_back(Cell("Name EN"));
    formatter.header().push_back(Cell("Name DE"));
    formatter.header().push_back(Cell("Square", Align::RIGHT));

    formatter.rows() = {
	{ "0", "zero", "Null", "0" },
	{ "1", "one", "Eins", "1"} ,
	{ "5", "five", "Fünf", "25" },
	{ "12", "twelve", "Zwölf", "144" }
    };

    string result = {
	"Number | Name EN | Name DE | Square\n"
	"-------+---------+---------+-------\n"
	"     0 | zero    | Null    |      0\n"
	"     1 | one     | Eins    |      1\n"
	"     5 | five    | Fünf    |     25\n"
	"    12 | twelve  | Zwölf   |    144\n"
    };

    BOOST_CHECK_EQUAL(str(formatter), result);
}
