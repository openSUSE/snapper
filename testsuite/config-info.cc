
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE config_info

#include <boost/test/unit_test.hpp>

#include <snapper/ConfigInfo.h>
#include <snapper/Snapper.h>
#include <snapper/SnapperDefines.h>

using namespace snapper;


// The config file is generated below the root prefix.

#define ROOT_PREFIX "config-info.root"


static void
create_config()
{
    BOOST_REQUIRE_EQUAL(system("mkdir -p " ROOT_PREFIX CONFIGS_DIR), 0);

    FILE* fout = fopen(ROOT_PREFIX CONFIGS_DIR "/test", "w");
    BOOST_REQUIRE(fout);
    fputs("SUBVOLUME=\"/\"\n"
	  "NUMBER_CLEANUP=\"yes\"\n"
	  "NUMBER_LIMIT=\"50\"\n", fout);
    fclose(fout);
}


BOOST_AUTO_TEST_CASE(boolean_values)
{
    create_config();

    ConfigInfo config_info("test", ROOT_PREFIX);

    bool tmp_bool;

    config_info.set_value("NUMBER_CLEANUP", "no");
    BOOST_CHECK(config_info.get_value("NUMBER_CLEANUP", tmp_bool));
    BOOST_CHECK_EQUAL(tmp_bool, false);

    config_info.set_value("NUMBER_CLEANUP", "yes");
    BOOST_CHECK(config_info.get_value("NUMBER_CLEANUP", tmp_bool));
    BOOST_CHECK_EQUAL(tmp_bool, true);

    config_info.set_value("TIMELINE_CREATE", false);
    BOOST_CHECK(config_info.get_value("TIMELINE_CREATE", tmp_bool));
    BOOST_CHECK_EQUAL(tmp_bool, false);

    BOOST_CHECK_THROW(config_info.set_value("NUMBER_CLEANUP", "YES"), InvalidConfigdataException);
    BOOST_CHECK_THROW(config_info.set_value("NUMBER_CLEANUP", "true"), InvalidConfigdataException);
    BOOST_CHECK_THROW(config_info.set_value("NUMBER_CLEANUP", ""), InvalidConfigdataException);
    BOOST_CHECK_THROW(config_info.set_value("TIMELINE_CREATE", "1"), InvalidConfigdataException);
    BOOST_CHECK_THROW(config_info.set_value("EMPTY_PRE_POST_CLEANUP", "No"), InvalidConfigdataException);

    // the value must not be modified by a failed set_value
    string tmp_string;
    BOOST_CHECK(config_info.get_value("NUMBER_CLEANUP", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "yes");
}


BOOST_AUTO_TEST_CASE(number_values)
{
    create_config();

    ConfigInfo config_info("test", ROOT_PREFIX);

    string tmp_string;

    config_info.set_value("NUMBER_MIN_AGE", "4000");
    BOOST_CHECK(config_info.get_value("NUMBER_MIN_AGE", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "4000");

    BOOST_CHECK_THROW(config_info.set_value("NUMBER_MIN_AGE", ""), InvalidConfigdataException);
    BOOST_CHECK_THROW(config_info.set_value("NUMBER_MIN_AGE", "4000s"), InvalidConfigdataException);
}


BOOST_AUTO_TEST_CASE(range_values)
{
    create_config();

    ConfigInfo config_info("test", ROOT_PREFIX);

    string tmp_string;

    config_info.set_value("NUMBER_LIMIT", "25");
    BOOST_CHECK(config_info.get_value("NUMBER_LIMIT", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "25");

    config_info.set_value("NUMBER_LIMIT", "25-50");
    BOOST_CHECK(config_info.get_value("NUMBER_LIMIT", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "25-50");

    BOOST_CHECK_THROW(config_info.set_value("NUMBER_LIMIT", ""), InvalidConfigdataException);
    BOOST_CHECK_THROW(config_info.set_value("NUMBER_LIMIT", "15-"), InvalidConfigdataException);
    BOOST_CHECK_THROW(config_info.set_value("NUMBER_LIMIT", "-40"), InvalidConfigdataException);
}


BOOST_AUTO_TEST_CASE(other_values)
{
    create_config();

    ConfigInfo config_info("test", ROOT_PREFIX);

    config_info.set_value("ALLOW_USERS", "linux tux");

    string tmp_string;
    BOOST_CHECK(config_info.get_value("ALLOW_USERS", tmp_string));
    BOOST_CHECK_EQUAL(tmp_string, "linux tux");
}


BOOST_AUTO_TEST_CASE(check_key_value)
{
    create_config();

    ConfigInfo config_info("test", ROOT_PREFIX);

    BOOST_CHECK_NO_THROW(config_info.check_key_value("SYNC_ACL", "yes"));
    BOOST_CHECK_NO_THROW(config_info.check_key_value("UNIQUE_NUMBERS", "no"));
    BOOST_CHECK_NO_THROW(config_info.check_key_value("ALLOW_USERS", "Tux"));

    BOOST_CHECK_THROW(config_info.check_key_value("SYNC_ACL", "YES"), InvalidConfigdataException);
    BOOST_CHECK_THROW(config_info.check_key_value("NUMBER_LIMIT", "25+"), InvalidConfigdataException);
    BOOST_CHECK_THROW(config_info.check_key_value("number_cleanup", "yes"), InvalidConfigdataException);
    BOOST_CHECK_THROW(config_info.check_key_value("SUBVOLUME", "/home"), InvalidConfigdataException);
}
