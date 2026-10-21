
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE rollback_backup_limit

#include <boost/test/unit_test.hpp>

#include "config.h"
#include <snapper/Btrfs.h>
#include <snapper/Snapper.h>

using namespace snapper;


BOOST_AUTO_TEST_CASE(valid)
{
    // empty (the default) keeps all backups
    BOOST_CHECK_EQUAL(parse_rollback_backup_limit(""), 0);
    BOOST_CHECK_EQUAL(parse_rollback_backup_limit("0"), 0);
    BOOST_CHECK_EQUAL(parse_rollback_backup_limit("1"), 1);
    BOOST_CHECK_EQUAL(parse_rollback_backup_limit("10"), 10);
}


BOOST_AUTO_TEST_CASE(invalid)
{
    // std::stoul would accept the first three (as 10, a huge number and 3)
    BOOST_CHECK_THROW(parse_rollback_backup_limit("10abc"), InvalidConfigException);
    BOOST_CHECK_THROW(parse_rollback_backup_limit("-1"), InvalidConfigException);
    BOOST_CHECK_THROW(parse_rollback_backup_limit(" 3"), InvalidConfigException);
    BOOST_CHECK_THROW(parse_rollback_backup_limit("ten"), InvalidConfigException);
    BOOST_CHECK_THROW(parse_rollback_backup_limit("99999999999999999999"), InvalidConfigException);
}
