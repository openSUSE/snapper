#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE snapper

#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <boost/test/unit_test.hpp>

#include "../snapper/QGroupRescanLimiter.h"

using namespace snapper;


namespace
{

    void
    remove_state_file(const std::string& state_dir, const std::string& filesystem_uuid)
    {
        int state_result = unlink((state_dir + "/" + filesystem_uuid).c_str());
        BOOST_CHECK(state_result == 0 || (state_result < 0 && errno == ENOENT));
        BOOST_CHECK_EQUAL(unlink((state_dir + "/" + filesystem_uuid + ".lock").c_str()), 0);
    }

    class Timezone
    {
    public:
        Timezone()
        : had_timezone(getenv("TZ") != nullptr),
          old_timezone(had_timezone ? getenv("TZ") : "")
        {
            if (setenv("TZ", "UTC", 1) < 0)
                throw std::runtime_error("failed to set timezone for qgroup rescan test");
            tzset();
        }

        ~Timezone()
        {
            if (had_timezone)
                setenv("TZ", old_timezone.c_str(), 1);
            else
                unsetenv("TZ");
            tzset();
        }

    private:
        bool had_timezone;
        std::string old_timezone;
    };

}


BOOST_AUTO_TEST_CASE(test_shared_qgroup_rescan_policy_must_match_per_filesystem)
{
    const std::string filesystem_a = "01234567-89ab-cdef-0123-456789abcdef";
    const std::string filesystem_b = "11234567-89ab-cdef-0123-456789abcdef";
    const std::string window = "05:00-21:00";
    const std::string hold = "/run/snapper/qgroup-rescan-hold-until";

    BOOST_CHECK(shared_policy_matches({{filesystem_a, window, hold, true}}, filesystem_a,
                                      window, hold, true));
    BOOST_CHECK(shared_policy_matches({{filesystem_a, window, hold, true},
                                       {filesystem_b, "00:00-24:00", "", true}},
                                      filesystem_a, window, hold, true));
    BOOST_CHECK(!shared_policy_matches({{filesystem_a, window, hold, true},
                                        {filesystem_a, "00:00-24:00", hold, true}},
                                       filesystem_a, window, hold, true));
    BOOST_CHECK(!shared_policy_matches({{filesystem_a, window, hold, true},
                                        {filesystem_a, window, "/run/other-hold", true}},
                                       filesystem_a, window, hold, true));
    BOOST_CHECK(!shared_policy_matches({{filesystem_b, window, hold, true}}, filesystem_a,
                                       window, hold, true));
    BOOST_CHECK(!shared_policy_matches({{filesystem_a, window, hold, false}}, filesystem_a,
                                       window, hold, true));
}


BOOST_AUTO_TEST_CASE(test_qgroup_rescan_limit_is_per_filesystem_and_persistent)
{
    char state_dir[] = "/var/tmp/snapper-qgroup-rescan-XXXXXX";
    BOOST_REQUIRE(mkdtemp(state_dir) != nullptr);

    const std::string filesystem_a = "01234567-89ab-cdef-0123-456789abcdef";
    const std::string filesystem_b = "11234567-89ab-cdef-0123-456789abcdef";
    QGroupRescanLimiter limiter(state_dir);
    time_t next_allowed = 0;

    BOOST_CHECK(limiter.reserve(filesystem_a, 1000, next_allowed));
    BOOST_CHECK_EQUAL(next_allowed, 1001 + QGroupRescanLimiter::interval);
    {
        std::ifstream state(std::string(state_dir) + "/" + filesystem_a);
        std::string version;
        time_t recorded = 0;
        state >> version >> recorded;
        BOOST_CHECK_EQUAL(version, "v1");
        BOOST_CHECK_EQUAL(recorded, 1001);
    }

    QGroupRescanLimiter another_instance(state_dir);
    BOOST_CHECK(!another_instance.reserve(filesystem_a, 1001, next_allowed));
    BOOST_CHECK_EQUAL(next_allowed, 1001 + QGroupRescanLimiter::interval);

    BOOST_CHECK(another_instance.reserve(filesystem_b, 1001, next_allowed));
    BOOST_CHECK(!limiter.reserve(filesystem_a,
                     1000 + QGroupRescanLimiter::interval,
                     next_allowed));
    BOOST_CHECK(limiter.reserve(filesystem_a,
                    1001 + QGroupRescanLimiter::interval,
                    next_allowed));

    remove_state_file(state_dir, filesystem_a);
    remove_state_file(state_dir, filesystem_b);
    BOOST_CHECK_EQUAL(rmdir(state_dir), 0);
}


BOOST_AUTO_TEST_CASE(test_qgroup_rescan_limit_fails_closed_on_oversized_state)
{
    char state_dir[] = "/var/tmp/snapper-qgroup-rescan-XXXXXX";
    BOOST_REQUIRE(mkdtemp(state_dir) != nullptr);

    const std::string filesystem_uuid = "01234567-89ab-cdef-0123-456789abcdef";
    const std::string state_path = std::string(state_dir) + "/" + filesystem_uuid;
    {
        std::ofstream state(state_path);
        state << "v1 1000\n" << std::string(80, 'x');
    }

    QGroupRescanLimiter limiter(state_dir);
    time_t next_allowed = 0;
    BOOST_CHECK_THROW(limiter.reserve(filesystem_uuid, 1000, next_allowed), std::runtime_error);

    BOOST_CHECK_EQUAL(unlink(state_path.c_str()), 0);
    BOOST_CHECK_EQUAL(unlink((state_path + ".lock").c_str()), 0);
    BOOST_CHECK_EQUAL(rmdir(state_dir), 0);
}


BOOST_AUTO_TEST_CASE(test_qgroup_rescan_limit_fails_closed_on_empty_state)
{
    char state_dir[] = "/var/tmp/snapper-qgroup-rescan-XXXXXX";
    BOOST_REQUIRE(mkdtemp(state_dir) != nullptr);

    const std::string filesystem_uuid = "01234567-89ab-cdef-0123-456789abcdef";
    const std::string state_path = std::string(state_dir) + "/" + filesystem_uuid;
    std::ofstream state(state_path);
    state.close();

    QGroupRescanLimiter limiter(state_dir);
    time_t next_allowed = 0;
    BOOST_CHECK_THROW(limiter.reserve(filesystem_uuid, 1000, next_allowed), std::runtime_error);

    BOOST_CHECK_EQUAL(unlink(state_path.c_str()), 0);
    BOOST_CHECK_EQUAL(unlink((state_path + ".lock").c_str()), 0);
    BOOST_CHECK_EQUAL(rmdir(state_dir), 0);
}


BOOST_AUTO_TEST_CASE(test_qgroup_rescan_limit_obeys_local_daily_window)
{
    Timezone timezone;
    char state_dir[] = "/var/tmp/snapper-qgroup-rescan-XXXXXX";
    BOOST_REQUIRE(mkdtemp(state_dir) != nullptr);

    const std::string filesystem_a = "01234567-89ab-cdef-0123-456789abcdef";
    const std::string filesystem_b = "11234567-89ab-cdef-0123-456789abcdef";
    const std::string filesystem_c = "21234567-89ab-cdef-0123-456789abcdef";
    const std::string filesystem_d = "31234567-89ab-cdef-0123-456789abcdef";
    const std::string filesystem_e = "41234567-89ab-cdef-0123-456789abcdef";
    QGroupRescanLimiter limiter(state_dir);
    const std::string window = "05:00-21:00";
    time_t next_allowed = 0;

    BOOST_CHECK(!limiter.reserve(filesystem_a, 4 * 3600 + 59 * 60,
                 next_allowed, window));
    BOOST_CHECK_EQUAL(next_allowed, 5 * 3600);
    BOOST_CHECK(limiter.reserve(filesystem_a, 5 * 3600, next_allowed, window));
    BOOST_CHECK_EQUAL(next_allowed, 24 * 3600 + 5 * 3600 + 1);
    BOOST_CHECK(!limiter.reserve(filesystem_a, 24 * 3600 + 4 * 3600 + 59 * 60,
                 next_allowed, window));
    BOOST_CHECK_EQUAL(next_allowed, 24 * 3600 + 5 * 3600 + 1);

    BOOST_CHECK(!limiter.reserve(filesystem_b, 21 * 3600, next_allowed, window));
    BOOST_CHECK_EQUAL(next_allowed, 24 * 3600 + 5 * 3600);
    BOOST_CHECK(limiter.reserve(filesystem_c, 20 * 3600, next_allowed, window));
    BOOST_CHECK_EQUAL(next_allowed, 24 * 3600 + 20 * 3600 + 1);
    BOOST_CHECK(limiter.reserve(filesystem_d, 23 * 3600,
                    next_allowed, "21:00-05:00"));
    BOOST_CHECK(limiter.reserve(filesystem_e, 21 * 3600,
                    next_allowed, window, true));
    BOOST_CHECK_EQUAL(next_allowed, 48 * 3600 + 5 * 3600);

    BOOST_CHECK_THROW(QGroupRescanLimiter::validate_window("24:00-05:00"),
              std::runtime_error);
    BOOST_CHECK_THROW(QGroupRescanLimiter::validate_window("05:00-05:00"),
              std::runtime_error);

    for (const std::string& filesystem_uuid :
        { filesystem_a, filesystem_b, filesystem_c, filesystem_d, filesystem_e })
        remove_state_file(state_dir, filesystem_uuid);
    BOOST_CHECK_EQUAL(rmdir(state_dir), 0);
}


BOOST_AUTO_TEST_CASE(test_qgroup_rescan_hold_file)
{
    char hold_file[] = "/var/tmp/snapper-qgroup-hold-XXXXXX";
    int fd = mkstemp(hold_file);
    BOOST_REQUIRE(fd >= 0);
    const std::string deadline = "2000\n";
    BOOST_REQUIRE_EQUAL(write(fd, deadline.data(), deadline.size()),
                            static_cast<ssize_t>(deadline.size()));
    BOOST_REQUIRE_EQUAL(close(fd), 0);

    time_t hold_until = 0;
    BOOST_CHECK(QGroupRescanLimiter::hold_active(hold_file, 1000, hold_until));
    BOOST_CHECK_EQUAL(hold_until, 2000);
    BOOST_CHECK(!QGroupRescanLimiter::hold_active(hold_file, 2000, hold_until));
    BOOST_CHECK_EQUAL(hold_until, 2000);
    BOOST_CHECK(!QGroupRescanLimiter::hold_active(std::string(hold_file) + ".missing",
                                                 1000, hold_until));

    fd = open(hold_file, O_WRONLY | O_TRUNC);
    BOOST_REQUIRE(fd >= 0);
    const std::string malformed = "tomorrow\n";
    BOOST_REQUIRE_EQUAL(write(fd, malformed.data(), malformed.size()),
                            static_cast<ssize_t>(malformed.size()));
    BOOST_REQUIRE_EQUAL(close(fd), 0);
    BOOST_CHECK_THROW(QGroupRescanLimiter::hold_active(hold_file, 1000, hold_until),
                      std::runtime_error);
    BOOST_REQUIRE_EQUAL(chmod(hold_file, 0666), 0);
    BOOST_CHECK_THROW(QGroupRescanLimiter::hold_active(hold_file, 1000, hold_until),
                      std::runtime_error);
    BOOST_REQUIRE_EQUAL(chmod(hold_file, 0600), 0);

    const std::string symlink_file = std::string(hold_file) + ".link";
    BOOST_REQUIRE_EQUAL(symlink(hold_file, symlink_file.c_str()), 0);
    BOOST_CHECK_THROW(QGroupRescanLimiter::hold_active(symlink_file, 1000, hold_until),
                      std::runtime_error);
    BOOST_CHECK_EQUAL(unlink(symlink_file.c_str()), 0);

    BOOST_CHECK_THROW(QGroupRescanLimiter::hold_active("relative-hold-file", 1000,
                                                      hold_until),
                      std::runtime_error);
    BOOST_CHECK_EQUAL(unlink(hold_file), 0);
}


BOOST_AUTO_TEST_CASE(test_qgroup_rescan_limit_serializes_concurrent_reservations)
{
    char state_dir[] = "/var/tmp/snapper-qgroup-rescan-XXXXXX";
    BOOST_REQUIRE(mkdtemp(state_dir) != nullptr);

    int gate[2];
    BOOST_REQUIRE_EQUAL(pipe(gate), 0);

    const std::string filesystem_uuid = "01234567-89ab-cdef-0123-456789abcdef";
    const unsigned int child_count = 4;
    std::vector<pid_t> children;
    for (unsigned int i = 0; i < child_count; ++i)
    {
        pid_t child = fork();
        BOOST_REQUIRE(child >= 0);
        if (child == 0)
        {
            close(gate[1]);
            char token;
            if (read(gate[0], &token, 1) != 1)
                _exit(2);
            try
            {
                QGroupRescanLimiter limiter(state_dir);
                time_t next_allowed = 0;
                _exit(limiter.reserve(filesystem_uuid, 1000, next_allowed) ? 0 : 1);
            }
            catch (...)
            {
                _exit(2);
            }
        }
        children.push_back(child);
    }

    close(gate[0]);
    const std::string tokens(child_count, 'x');
    BOOST_REQUIRE_EQUAL(write(gate[1], tokens.data(), tokens.size()),
            static_cast<ssize_t>(tokens.size()));
    close(gate[1]);

    unsigned int reservations = 0;
    for (pid_t child : children)
    {
        int status = 0;
        BOOST_REQUIRE_EQUAL(waitpid(child, &status, 0), child);
        BOOST_REQUIRE(WIFEXITED(status));
        BOOST_REQUIRE(WEXITSTATUS(status) <= 1);
        reservations += WEXITSTATUS(status) == 0;
    }

    BOOST_CHECK_EQUAL(reservations, 1);

    remove_state_file(state_dir, filesystem_uuid);
    BOOST_CHECK_EQUAL(rmdir(state_dir), 0);
}


BOOST_AUTO_TEST_CASE(test_active_rescan_refreshes_filesystem_cooldown)
{
    char state_dir[] = "/var/tmp/snapper-qgroup-rescan-XXXXXX";
    BOOST_REQUIRE(mkdtemp(state_dir) != nullptr);

    const std::string filesystem_uuid = "01234567-89ab-cdef-0123-456789abcdef";
    QGroupRescanLimiter limiter(state_dir);
    time_t next_allowed = 0;

    BOOST_CHECK(limiter.reserve(filesystem_uuid, 1000, next_allowed));
    BOOST_CHECK(limiter.reserve(filesystem_uuid, 1001, next_allowed,
                    "05:00-21:00", true));
    BOOST_CHECK(!limiter.reserve(filesystem_uuid, 1002, next_allowed));
    BOOST_CHECK_EQUAL(next_allowed, 1002 + QGroupRescanLimiter::interval);

    const std::string filesystem_unlimited =
        "21234567-89ab-cdef-0123-456789abcdef";
    BOOST_CHECK(limiter.reserve(filesystem_unlimited, 1000, next_allowed,
                                "00:00-24:00", false, false));
    BOOST_CHECK(limiter.reserve(filesystem_unlimited, 1001, next_allowed,
                                "00:00-24:00", false, false));

    remove_state_file(state_dir, filesystem_uuid);
    remove_state_file(state_dir, filesystem_unlimited);
    BOOST_CHECK_EQUAL(rmdir(state_dir), 0);
}
