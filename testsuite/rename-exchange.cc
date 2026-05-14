
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
