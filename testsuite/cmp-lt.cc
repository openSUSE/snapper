
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <locale>
#include <boost/test/unit_test.hpp>

#include <snapper/File.h>

using namespace snapper;


namespace std
{
    std::ostream&
    operator<<(std::ostream& s, const vector<string>& v)
    {
	for(std::vector<string>::const_iterator it = v.begin(); it != v.end(); ++it)
	{
	    if (it != v.begin())
		s << " ";
	    s << *it;
	}

	return s;
    }
}


BOOST_AUTO_TEST_CASE(test1)
{
    std::locale::global(std::locale("C"));

    vector<string> v = { "A", "B", "b", "a" };
    sort(v.begin(), v.end(), File::cmp_lt);

    BOOST_CHECK_EQUAL(v, vector<string>({ "A", "B", "a", "b" }));
}


BOOST_AUTO_TEST_CASE(test2)
{
    std::locale::global(std::locale("en_US.UTF-8"));

    vector<string> v = { "A", "B", "b", "a" };
    sort(v.begin(), v.end(), File::cmp_lt);

    BOOST_CHECK_EQUAL(v, vector<string>({ "a", "A", "b", "B" }));
}


BOOST_AUTO_TEST_CASE(test3)
{
    std::locale::global(std::locale("de_DE.UTF-8"));

    vector<string> v = { "a", "b", "ä" };
    sort(v.begin(), v.end(), File::cmp_lt);

    BOOST_CHECK_EQUAL(v, vector<string>({ "a", "ä", "b" }));
}


BOOST_AUTO_TEST_CASE(test4)
{
    std::locale::global(std::locale("en_US.UTF-8"));

    vector<string> v = { "a", "\344" }; // invalid UTF-8
    sort(v.begin(), v.end(), File::cmp_lt);

    BOOST_CHECK_EQUAL(v, vector<string>({ "\344", "a" }));
}
