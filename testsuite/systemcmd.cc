
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE systemcmd

#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <vector>

#include <snapper/SystemCmd.h>


using namespace std;
using namespace snapper;


BOOST_AUTO_TEST_CASE(test_sync_stdout)
{
    SystemCmd cmd({ "echo", "hello snapper" });
    BOOST_CHECK_EQUAL(cmd.retcode(), 0);
    BOOST_REQUIRE_EQUAL(cmd.stdout().size(), 1);
    BOOST_CHECK_EQUAL(cmd.stdout()[0], "hello snapper");
    BOOST_CHECK(cmd.stderr().empty());
}


BOOST_AUTO_TEST_CASE(test_sync_stderr)
{
    SystemCmd cmd({ "sh", "-c", "echo hello error >&2" });
    BOOST_CHECK_EQUAL(cmd.retcode(), 0);
    BOOST_CHECK(cmd.stdout().empty());
    BOOST_REQUIRE_EQUAL(cmd.stderr().size(), 1);
    BOOST_CHECK_EQUAL(cmd.stderr()[0], "hello error");
}


BOOST_AUTO_TEST_CASE(test_sync_multiline)
{
    SystemCmd cmd({ "sh", "-c", "echo line1; echo line2; echo -n line3" });
    BOOST_CHECK_EQUAL(cmd.retcode(), 0);
    BOOST_REQUIRE_EQUAL(cmd.stdout().size(), 3);
    BOOST_CHECK_EQUAL(cmd.stdout()[0], "line1");
    BOOST_CHECK_EQUAL(cmd.stdout()[1], "line2");
    BOOST_CHECK_EQUAL(cmd.stdout()[2], "line3");
}


BOOST_AUTO_TEST_CASE(test_exit_status)
{
    SystemCmd cmd({ "sh", "-c", "exit 42" });
    BOOST_CHECK_EQUAL(cmd.retcode(), 42);
}


BOOST_AUTO_TEST_CASE(test_not_found)
{
    SystemCmd cmd({ "/bin/does/not/exist" });
    BOOST_CHECK_EQUAL(cmd.retcode(), 127);
}


BOOST_AUTO_TEST_CASE(test_async_lifecycle)
{
    // Start an async process reading stdin; destructor closes stdin and reaps child

    SystemCmd cmd({ "cat" }, SystemCmd::StartMode::ASYNC);
}


BOOST_AUTO_TEST_CASE(test_quote)
{
    BOOST_CHECK_EQUAL(SystemCmd::quote("hello"), "'hello'");
    BOOST_CHECK_EQUAL(SystemCmd::quote("hello world"), "'hello world'");
    BOOST_CHECK_EQUAL(SystemCmd::quote("don't"), "'don'\\''t'");
}
