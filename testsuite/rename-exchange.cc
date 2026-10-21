
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE exchange

#include <boost/test/unit_test.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include <snapper/FileUtils.h>

using namespace std;
using namespace snapper;


struct TestTmpDir
{
    TestTmpDir()
    {
	char tmpl[] = "/tmp/snapper-rename-exchange-test-XXXXXX";
	path = mkdtemp(tmpl);
	BOOST_REQUIRE(!path.empty());
    }

    ~TestTmpDir()
    {
	// clean up subdirs if still present
	rmdir((path + "/a").c_str());
	rmdir((path + "/b").c_str());
	rmdir(path.c_str());
    }

    string path;
};


BOOST_AUTO_TEST_CASE(exchange_swaps_dirs)
{
    TestTmpDir tmp;

    // create two subdirs with a sentinel file each to verify identity after swap
    BOOST_REQUIRE(mkdir((tmp.path + "/a").c_str(), 0755) == 0);
    BOOST_REQUIRE(mkdir((tmp.path + "/b").c_str(), 0755) == 0);

    // write a sentinel into each: /a/a-marker and /b/b-marker
    int fa = open((tmp.path + "/a/a-marker").c_str(), O_CREAT | O_WRONLY, 0644);
    BOOST_REQUIRE(fa >= 0);
    close(fa);
    int fb = open((tmp.path + "/b/b-marker").c_str(), O_CREAT | O_WRONLY, 0644);
    BOOST_REQUIRE(fb >= 0);
    close(fb);

    SDir base(tmp.path);
    int ret = base.exchange("a", "b");
    BOOST_REQUIRE_EQUAL(ret, 0);

    // after exchange: /a should contain b-marker, /b should contain a-marker
    BOOST_CHECK_EQUAL(access((tmp.path + "/a/b-marker").c_str(), F_OK), 0);
    BOOST_CHECK_EQUAL(access((tmp.path + "/b/a-marker").c_str(), F_OK), 0);

    // clean up marker files
    unlink((tmp.path + "/a/b-marker").c_str());
    unlink((tmp.path + "/b/a-marker").c_str());
}


BOOST_AUTO_TEST_CASE(exchange_nonexistent_fails)
{
    TestTmpDir tmp;

    BOOST_REQUIRE(mkdir((tmp.path + "/a").c_str(), 0755) == 0);

    SDir base(tmp.path);
    // "b" does not exist — RENAME_EXCHANGE requires both to exist
    int ret = base.exchange("a", "b");
    BOOST_CHECK(ret != 0);

    rmdir((tmp.path + "/a").c_str());
}


BOOST_AUTO_TEST_CASE(rename_moves_dir_between_directories)
{
    TestTmpDir tmp;

    // /a/sub with a sentinel file is moved into /b
    BOOST_REQUIRE(mkdir((tmp.path + "/a").c_str(), 0755) == 0);
    BOOST_REQUIRE(mkdir((tmp.path + "/b").c_str(), 0755) == 0);
    BOOST_REQUIRE(mkdir((tmp.path + "/a/sub").c_str(), 0755) == 0);

    int fd = open((tmp.path + "/a/sub/marker").c_str(), O_CREAT | O_WRONLY, 0644);
    BOOST_REQUIRE(fd >= 0);
    close(fd);

    {
	SDir a(tmp.path + "/a");
	SDir b(tmp.path + "/b");

	int ret = a.rename("sub", b, "sub");
	BOOST_REQUIRE_EQUAL(ret, 0);
    }

    BOOST_CHECK_EQUAL(access((tmp.path + "/b/sub/marker").c_str(), F_OK), 0);
    BOOST_CHECK(access((tmp.path + "/a/sub").c_str(), F_OK) != 0);

    // clean up
    unlink((tmp.path + "/b/sub/marker").c_str());
    rmdir((tmp.path + "/b/sub").c_str());
}


BOOST_AUTO_TEST_CASE(exchange_across_directories)
{
    TestTmpDir tmp;

    // /a/x holds a-marker, /b/y holds b-marker; an atomic cross-directory
    // exchange must swap them: afterwards /a/x holds b-marker and /b/y holds
    // a-marker. Needs the flags parameter on the cross-directory rename overload.
    BOOST_REQUIRE(mkdir((tmp.path + "/a").c_str(), 0755) == 0);
    BOOST_REQUIRE(mkdir((tmp.path + "/b").c_str(), 0755) == 0);
    BOOST_REQUIRE(mkdir((tmp.path + "/a/x").c_str(), 0755) == 0);
    BOOST_REQUIRE(mkdir((tmp.path + "/b/y").c_str(), 0755) == 0);

    int fa = open((tmp.path + "/a/x/a-marker").c_str(), O_CREAT | O_WRONLY, 0644);
    BOOST_REQUIRE(fa >= 0);
    close(fa);
    int fb = open((tmp.path + "/b/y/b-marker").c_str(), O_CREAT | O_WRONLY, 0644);
    BOOST_REQUIRE(fb >= 0);
    close(fb);

    {
	SDir a(tmp.path + "/a");
	SDir b(tmp.path + "/b");

	int ret = a.rename("x", b, "y", RENAME_EXCHANGE);
	BOOST_REQUIRE_EQUAL(ret, 0);
    }

    BOOST_CHECK_EQUAL(access((tmp.path + "/a/x/b-marker").c_str(), F_OK), 0);
    BOOST_CHECK_EQUAL(access((tmp.path + "/b/y/a-marker").c_str(), F_OK), 0);

    // clean up
    unlink((tmp.path + "/a/x/b-marker").c_str());
    unlink((tmp.path + "/b/y/a-marker").c_str());
    rmdir((tmp.path + "/a/x").c_str());
    rmdir((tmp.path + "/b/y").c_str());
}
