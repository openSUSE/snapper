
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include <iostream>
#include <numeric>
#include <iomanip>

#include "../client/utils/Table.h"


using namespace std;


void
check(const Table& table, const vector<string>& output)
{
    ostringstream tmp;
    tmp << setw(42) << table;
    string lhs = tmp.str();

    string rhs = accumulate(output.begin(), output.end(), (string)(""),
                            [](const string& a, const string& b) { return a + b + '\n'; });

    BOOST_CHECK_EQUAL(lhs, rhs);
}


BOOST_AUTO_TEST_CASE(test1)
{
    locale::global(locale("en_GB.UTF-8"));

    Table table;

    TableHeader header;
    header.add("Number", TableAlign::RIGHT);
    header.add("Name EN");
    header.add("Name DE");
    header.add("Square", TableAlign::RIGHT);
    table.setHeader(header);

    TableRow row1;
    row1.add("0");
    row1.add("zero");
    row1.add("Null");
    row1.add("0");
    table.add(row1);

    TableRow row2;
    row2 << "1" << "one" << "Eins" << "1";
    table.add(row2);

    TableRow row3;
    row3 << "5" << "five" << "Fünf" << "25";
    table.add(row3);

    TableRow row4;
    row4 << "12" << "twelve" << "Zwölf" << "144";
    table.add(row4);

    vector<string> output = {
	"Number | Name EN | Name DE | Square",
	"-------+---------+---------+-------",
	"     0 | zero    | Null    |      0",
	"     1 | one     | Eins    |      1",
	"     5 | five    | Fünf    |     25",
	"    12 | twelve  | Zwölf   |    144"
    };

    check(table, output);
}
