
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <boost/test/unit_test.hpp>

#include <iostream>
#include <numeric>
#include <iomanip>

#include "../client/utils/Table.h"


using namespace std;
using namespace snapper;


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
    Table table({ "A", "B" });

    table.set_style(Style::LIGHT);

    Table::Row row1(table, { "a" });
    table.add(row1);

    Table::Row row2(table, { "a", "b" });
    table.add(row2);

    vector<string> output = {
	"A │ B",
	"──┼──",
	"a │",
	"a │ b"
    };

    check(table, output);
}


BOOST_AUTO_TEST_CASE(test2)
{
    Table table({ "A", "B" });

    Table::Row row1(table, { "a", "b", "c" });
    table.add(row1);

    BOOST_CHECK_EXCEPTION({ ostringstream tmp; tmp << table; }, runtime_error, [](const exception& e) {
        return strcmp(e.what(), "too many columns") == 0;
    });
}


BOOST_AUTO_TEST_CASE(test3)
{
    locale::global(locale("en_GB.UTF-8"));

    Table table({
	Cell("Number", Id::NUMBER, Align::RIGHT), Cell("Name EN"), Cell("Name DE"),
	Cell("Square", Align::RIGHT)
    });

    table.set_style(Style::ASCII);

    Table::Row row1(table, { "0", "zero", "Null", "0" });
    table.add(row1);

    Table::Row row2(table, { "1", "one", "Eins", "1" });
    table.add(row2);

    Table::Row row3(table, { "5", "five", "Fünf", "25" });
    table.add(row3);

    Table::Row row4(table, { "12", "twelve", "Zwölf", "144" });
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


BOOST_AUTO_TEST_CASE(test4)
{
    Table table({ Cell("Number", Align::RIGHT), Cell("Description", Id::DESCRIPTION) });

    table.set_style(Style::LIGHT);
    table.set_screen_width(25);
    table.set_abbreviate(Id::DESCRIPTION, true);

    Table::Row row1(table, { "1", "boot" });
    table.add(row1);

    Table::Row row2(table, { "2", "before the system update" });
    table.add(row2);

    Table::Row row3(table, { "3", "läuft schön rund" });
    table.add(row3);

    vector<string> output = {
	"Number │ Description",
	"───────┼─────────────────",
	"     1 │ boot",
	"     2 │ before the syst…",
	"     3 │ läuft schön rund"
    };

    check(table, output);
}
