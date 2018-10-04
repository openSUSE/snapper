
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include <iostream>
#include <numeric>

#include "../client/utils/Table.h"


using namespace std;


void
check(const Table& table, const vector<string>& output)
{
    ostringstream tmp;
    tmp << table;
    string lhs = tmp.str();

    string rhs = accumulate(output.begin(), output.end(), (string)(""),
                            [](const string& a, const string& b) { return a + b + '\n'; });

    BOOST_CHECK_EQUAL(lhs, rhs);
}


BOOST_AUTO_TEST_CASE(test1)
{
    Table table;

    TableHeader header;
    header.add("Number", TableAlign::RIGHT);
    header.add("Name");
    header.add("Square", TableAlign::RIGHT);
    table.setHeader(header);

    TableRow row1;
    row1.add("0");
    row1.add("zero");
    row1.add("0");
    table.add(row1);

    TableRow row2;
    row2 << "1" << "one" << "1";
    table.add(row2);

    TableRow row3;
    row3 << "4" << "four" << "16";
    table.add(row3);

    vector<string> output = {
	"Number | Name | Square",
	"-------+------+-------",
	"     0 | zero |      0",
	"     1 | one  |      1",
	"     4 | four |     16"
    };

    check(table, output);
}
