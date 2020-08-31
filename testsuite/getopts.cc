
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE getopt

#include <boost/test/unit_test.hpp>

#include "../client/utils/GetOpts.h"

using namespace snapper;


class Args
{

public:

    Args(std::initializer_list<string> init)
    {
	optind = 0;

	for (const string& s : init)
	    tmp.push_back(strdup(s.c_str()));
	tmp.push_back(nullptr);
    }

    ~Args()
    {
	for (char* s : tmp)
	    free(s);
    }

    int argc() const { return tmp.size() - 1; }
    char** argv() { return &tmp.front(); }

    bool all_used() const { return (size_t)(optind) == tmp.size() - 1; }

private:

    vector<char*> tmp;

};


const vector<Option> global_opts = {
    Option("verbose",		no_argument,		'v'),
    Option("table-style",	required_argument,	't')
};


const vector<Option> create_opts = {
    Option("type",		required_argument,	't'),
    Option("print-number",	no_argument,		'p'),
};


BOOST_AUTO_TEST_CASE(good1)
{
    Args args({ "getopt", "--verbose" });
    GetOpts get_opts(args.argc(), args.argv());

    ParsedOpts parsed_global_opts = get_opts.parse(global_opts);

    BOOST_CHECK(parsed_global_opts.has_option("verbose"));
    BOOST_CHECK(!parsed_global_opts.has_option("read-my-mind"));

    BOOST_CHECK(!get_opts.has_args());

    BOOST_CHECK(args.all_used());
}


BOOST_AUTO_TEST_CASE(good2)
{
    Args args({ "getopt", "-v", "create", "--type", "pre" });
    GetOpts get_opts(args.argc(), args.argv());

    // parse global options

    ParsedOpts parsed_global_opts = get_opts.parse(global_opts);

    BOOST_CHECK(parsed_global_opts.has_option("verbose"));
    BOOST_CHECK(!parsed_global_opts.has_option("dry-run"));

    // check that command is there

    BOOST_CHECK(get_opts.has_args());
    BOOST_CHECK_EQUAL(get_opts.pop_arg(), "create");

    // parse create options

    ParsedOpts parsed_create_opts = get_opts.parse("create", create_opts);

    BOOST_CHECK(parsed_create_opts.has_option("type"));

    ParsedOpts::const_iterator it = parsed_create_opts.find("type");
    BOOST_CHECK(it != parsed_create_opts.end());
    BOOST_CHECK(it->second == "pre");

    // check that no further command is there

    BOOST_CHECK(!get_opts.has_args());

    BOOST_CHECK(args.all_used());
}


BOOST_AUTO_TEST_CASE(good3)
{
    Args args({ "getopt", "create", "-tpre" });
    GetOpts get_opts(args.argc(), args.argv());

    // parse global options

    ParsedOpts parsed_global_opts = get_opts.parse(global_opts);

    // check that command is there

    BOOST_CHECK(get_opts.has_args());
    BOOST_CHECK_EQUAL(get_opts.pop_arg(), "create");

    // parse create options

    ParsedOpts parsed_create_opts = get_opts.parse("create", create_opts);

    BOOST_CHECK(parsed_create_opts.has_option("type"));

    ParsedOpts::const_iterator it = parsed_create_opts.find("type");
    BOOST_CHECK(it != parsed_create_opts.end());
    BOOST_CHECK(it->second == "pre");

    // check that no further command is there

    BOOST_CHECK(!get_opts.has_args());

    BOOST_CHECK(args.all_used());
}


BOOST_AUTO_TEST_CASE(error1)
{
    Args args({ "getopt", "--table-style" });
    GetOpts get_opts(args.argc(), args.argv());

    BOOST_CHECK_EXCEPTION(get_opts.parse(global_opts), OptionsException, [](const exception& e) {
	return strcmp(e.what(), "Missing argument for global option '--table-style'.") == 0;
    });
}


BOOST_AUTO_TEST_CASE(error2)
{
    Args args({ "getopt", "create", "--type" });
    GetOpts get_opts(args.argc(), args.argv());

    get_opts.parse(global_opts);
    get_opts.pop_arg();

    BOOST_CHECK_EXCEPTION(get_opts.parse("create", create_opts), OptionsException, [](const exception& e) {
	return strcmp(e.what(), "Missing argument for command option '--type'.") == 0;
    });
}


BOOST_AUTO_TEST_CASE(error3)
{
    Args args({ "getopt", "--read-my-mind" });
    GetOpts get_opts(args.argc(), args.argv());

    BOOST_CHECK_EXCEPTION(get_opts.parse(global_opts), OptionsException, [](const exception& e) {
	return strcmp(e.what(), "Unknown global option '--read-my-mind'.") == 0;
    });
}


BOOST_AUTO_TEST_CASE(error4)
{
    Args args({ "getopt", "create", "--read-my-mind" });
    GetOpts get_opts(args.argc(), args.argv());

    get_opts.parse(global_opts);
    get_opts.pop_arg();

    BOOST_CHECK_EXCEPTION(get_opts.parse("create", create_opts), OptionsException, [](const exception& e) {
	return strcmp(e.what(), "Unknown option '--read-my-mind' for command 'create'.") == 0;
    });
}


BOOST_AUTO_TEST_CASE(error5)
{
    Args args({ "getopt", "create", "-px" });
    GetOpts get_opts(args.argc(), args.argv());

    get_opts.parse(global_opts);
    get_opts.pop_arg();

    BOOST_CHECK_EXCEPTION(get_opts.parse("create", create_opts), OptionsException, [](const exception& e) {
	return strcmp(e.what(), "Unknown option '-x' for command 'create'.") == 0;
    });
}


BOOST_AUTO_TEST_CASE(error6)
{
    Args args({ "getopt", "create", "-pt" });
    GetOpts get_opts(args.argc(), args.argv());

    get_opts.parse(global_opts);
    get_opts.pop_arg();

    BOOST_CHECK_EXCEPTION(get_opts.parse("create", create_opts), OptionsException, [](const exception& e) {
	return strcmp(e.what(), "Missing argument for command option '--type'.") == 0;
    });
}
