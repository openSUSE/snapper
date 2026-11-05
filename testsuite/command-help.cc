#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE command_help

#include <boost/test/unit_test.hpp>

#include <cstdlib>
#include <string>

#include <snapper/SystemCmd.h>

using namespace std;
using namespace snapper;


// Runs the snapper client from the build tree. The DBus system bus is made unreachable
// to ensure that showing the help of a command needs neither DBus nor a config.

SystemCmd
run(const SystemCmd::Args& args)
{
    setenv("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/nonexistent", 1);

    SystemCmd::Args tmp = { "../client/snapper/snapper" };
    tmp << args.get_values();

    return SystemCmd(tmp);
}


BOOST_AUTO_TEST_CASE(command)
{
    SystemCmd cmd = run({ "create-config", "--help" });

    BOOST_CHECK_EQUAL(cmd.retcode(), 0);
    BOOST_REQUIRE(!cmd.stdout().empty());
    BOOST_CHECK_EQUAL(cmd.stdout()[0], "  Create config:");
    BOOST_CHECK(cmd.stderr().empty());
}


BOOST_AUTO_TEST_CASE(alias)
{
    SystemCmd cmd = run({ "rm", "--help" });

    BOOST_CHECK_EQUAL(cmd.retcode(), 0);
    BOOST_REQUIRE(!cmd.stdout().empty());
    BOOST_CHECK_EQUAL(cmd.stdout()[0], "  Delete snapshot:");
    BOOST_CHECK(cmd.stderr().empty());
}


BOOST_AUTO_TEST_CASE(global_options)
{
    SystemCmd cmd = run({ "--no-dbus", "--root", "/nonexistent", "get-config", "--help" });

    BOOST_CHECK_EQUAL(cmd.retcode(), 0);
    BOOST_REQUIRE(!cmd.stdout().empty());
    BOOST_CHECK_EQUAL(cmd.stdout()[0], "  Get config:");
    BOOST_CHECK(cmd.stderr().empty());
}


BOOST_AUTO_TEST_CASE(unknown_command)
{
    SystemCmd cmd = run({ "no-such-command", "--help" });

    BOOST_CHECK_EQUAL(cmd.retcode(), 1);
    BOOST_CHECK(cmd.stdout().empty());
    BOOST_REQUIRE(!cmd.stderr().empty());
    BOOST_CHECK_EQUAL(cmd.stderr()[0], "Unknown command 'no-such-command'.");
}
