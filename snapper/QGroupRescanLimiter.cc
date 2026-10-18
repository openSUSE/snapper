/*
 * Copyright (c) 2026 SUSE LLC
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 */

#include "QGroupRescanLimiter.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "snapper/AppUtil.h"

namespace snapper
{

    namespace
    {
	struct DailyWindow
	{
	    int start_minute;
	    int end_minute;
	};


	class FileDescriptor
	{
	public:
	    explicit FileDescriptor(int fd) : fd(fd) {}
	    ~FileDescriptor() { if (fd >= 0) close(fd); }
	    int get() const { return fd; }

	private:
	    int fd;
	};

	void
	check_owner_and_mode(int fd, bool directory)
	{
	    struct stat st;
	    if (fstat(fd, &st) < 0)
		throw runtime_error_with_errno("fstat(qgroup rescan state) failed", errno);

	    if ((directory ? !S_ISDIR(st.st_mode) : !S_ISREG(st.st_mode)) ||
		st.st_uid != geteuid() || (st.st_mode & 0022) != 0)
		throw std::runtime_error("unsafe qgroup rescan state ownership or mode");
	}

	bool
	valid_uuid(const std::string& uuid)
	{
	    if (uuid.empty())
		return false;

	    for (char c : uuid)
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
		      (c >= 'A' && c <= 'F') || c == '-'))
		    return false;

	    return true;
	}

	int
	parse_clock_time(const std::string& value, bool allow_end_of_day)
	{
	    if (value.size() != 5 || value[2] != ':' ||
		value[0] < '0' || value[0] > '9' ||
		value[1] < '0' || value[1] > '9' ||
		value[3] < '0' || value[3] > '9' ||
		value[4] < '0' || value[4] > '9')
		throw std::runtime_error("invalid QGROUP_RESCAN_WINDOW; expected HH:MM-HH:MM");

	    const int hour = (value[0] - '0') * 10 + value[1] - '0';
	    const int minute = (value[3] - '0') * 10 + value[4] - '0';
	    if (minute > 59 || hour > 24 || (hour == 24 && (!allow_end_of_day || minute != 0)))
		throw std::runtime_error("invalid QGROUP_RESCAN_WINDOW; expected HH:MM-HH:MM");

	    return hour * 60 + minute;
	}

	DailyWindow
	parse_window(const std::string& value)
	{
	    const size_t separator = value.find('-');
	    if (separator == std::string::npos || value.find('-', separator + 1) != std::string::npos)
		throw std::runtime_error("invalid QGROUP_RESCAN_WINDOW; expected HH:MM-HH:MM");

	    DailyWindow window = {
		parse_clock_time(value.substr(0, separator), false),
		parse_clock_time(value.substr(separator + 1), true)
	    };
	    if (window.start_minute == window.end_minute)
		throw std::runtime_error("invalid QGROUP_RESCAN_WINDOW; start and end must differ");

	    return window;
	}

	bool
	window_contains(time_t value, const DailyWindow& window)
	{
	    if (window.start_minute == 0 && window.end_minute == 24 * 60)
		return true;

	    struct tm local_time;
	    if (localtime_r(&value, &local_time) == nullptr)
		throw std::runtime_error("cannot convert qgroup rescan time to local time");

	    const int minute = local_time.tm_hour * 60 + local_time.tm_min;
	    if (window.start_minute < window.end_minute)
		return minute >= window.start_minute && minute < window.end_minute;

	    return minute >= window.start_minute || minute < window.end_minute;
	}

	time_t
	next_window_time(time_t after, const DailyWindow& window)
	{
	    if (window_contains(after, window))
		return after;

	    struct tm local_time;
	    if (localtime_r(&after, &local_time) == nullptr)
		throw std::runtime_error("cannot convert qgroup rescan time to local time");

	    for (int day = 0; day < 3; ++day)
	    {
		struct tm candidate = local_time;
		candidate.tm_mday += day;
		candidate.tm_hour = window.start_minute / 60;
		candidate.tm_min = window.start_minute % 60;
		candidate.tm_sec = 0;
		candidate.tm_isdst = -1;
		const time_t candidate_time = mktime(&candidate);
		if (candidate_time >= after && window_contains(candidate_time, window))
		    return candidate_time;
	    }

	    throw std::runtime_error("cannot find next qgroup rescan window");
	}

	time_t
	next_cooldown(time_t last_start)
	{
	    const int64_t start = static_cast<int64_t>(last_start);
	    const int64_t max_time = static_cast<int64_t>(std::numeric_limits<time_t>::max());
	    if (start > INT64_MAX - QGroupRescanLimiter::interval ||
		start > max_time - QGroupRescanLimiter::interval)
		return std::numeric_limits<time_t>::max();

	    return static_cast<time_t>(start + QGroupRescanLimiter::interval);
	}

    }


    QGroupRescanLimiter::QGroupRescanLimiter(const std::string& state_dir)
	: state_dir(state_dir)
    {
	if (mkdir(this->state_dir.c_str(), 0700) < 0 && errno != EEXIST)
	    throw runtime_error_with_errno("mkdir(qgroup rescan state) failed", errno);

	FileDescriptor dirfd(open(this->state_dir.c_str(), O_RDONLY | O_DIRECTORY |
				 O_CLOEXEC | O_NOFOLLOW));
	if (dirfd.get() < 0)
	    throw runtime_error_with_errno("open(qgroup rescan state) failed", errno);
	check_owner_and_mode(dirfd.get(), true);
    }


    bool
    QGroupRescanLimiter::reserve(const std::string& filesystem_uuid, time_t now,
				 time_t& next_allowed, const std::string& daily_window,
				 bool active_scan) const
    {
	if (!valid_uuid(filesystem_uuid))
	    throw std::runtime_error("invalid Btrfs filesystem UUID for qgroup rescan state");
	if (now < 0)
	    throw std::runtime_error("invalid time for qgroup rescan state");
	if (now == std::numeric_limits<time_t>::max())
	    throw std::runtime_error("time overflow for qgroup rescan state");
	const time_t reservation_time = now + 1;
	const DailyWindow window = parse_window(daily_window);

	FileDescriptor dirfd(open(state_dir.c_str(), O_RDONLY | O_DIRECTORY |
				 O_CLOEXEC | O_NOFOLLOW));
	if (dirfd.get() < 0)
	    throw runtime_error_with_errno("open(qgroup rescan state) failed", errno);
	check_owner_and_mode(dirfd.get(), true);

	const std::string lock_name = filesystem_uuid + ".lock";
	FileDescriptor lockfd(openat(dirfd.get(), lock_name.c_str(),
				     O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600));
	if (lockfd.get() < 0)
	    throw runtime_error_with_errno("openat(qgroup rescan lock) failed", errno);
	check_owner_and_mode(lockfd.get(), false);

	if (flock(lockfd.get(), LOCK_EX) < 0)
	    throw runtime_error_with_errno("flock(qgroup rescan lock) failed", errno);

	char contents[64] = {};
	int64_t last_start = 0;
	FileDescriptor statefd(openat(dirfd.get(), filesystem_uuid.c_str(),
				      O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
	if (statefd.get() >= 0)
	{
	    check_owner_and_mode(statefd.get(), false);
	    const ssize_t bytes = pread(statefd.get(), contents, sizeof(contents), 0);
	    if (bytes < 0)
		throw runtime_error_with_errno("read(qgroup rescan state) failed", errno);
	    if (bytes == 0 || bytes == static_cast<ssize_t>(sizeof(contents)))
		throw std::runtime_error("invalid qgroup rescan state; refusing to start another scan");

	    std::istringstream input(std::string(contents, bytes));
	    std::string version;
	    int64_t parsed = 0;
	    if (!(input >> version >> parsed) || version != "v1" || parsed < 0 ||
		parsed > static_cast<int64_t>(std::numeric_limits<time_t>::max()))
		throw std::runtime_error("invalid qgroup rescan state; refusing to start another scan");
	    input >> std::ws;
	    if (!input.eof())
		throw std::runtime_error("invalid qgroup rescan state; refusing to start another scan");
	    last_start = parsed;
	}
	else if (errno != ENOENT)
	    throw runtime_error_with_errno("openat(qgroup rescan state) failed", errno);

	const bool cooldown_active = last_start > 0 &&
	    (now < last_start || static_cast<int64_t>(now) - last_start < interval);
	if (!active_scan && cooldown_active)
	{
	    next_allowed = next_window_time(next_cooldown(static_cast<time_t>(last_start)), window);
	    return false;
	}
	if (!active_scan && !window_contains(now, window))
	{
	    next_allowed = next_window_time(now, window);
	    return false;
	}

	const std::string updated = "v1 " +
	    std::to_string(static_cast<int64_t>(reservation_time)) + "\n";
	std::string temporary_name;
	int temporary_fd = -1;
	for (unsigned int attempt = 0; attempt < 100; ++attempt)
	{
	    temporary_name = filesystem_uuid + ".tmp." + std::to_string(getpid()) + "." +
		std::to_string(attempt);
	    temporary_fd = openat(dirfd.get(), temporary_name.c_str(),
			   O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
	    if (temporary_fd >= 0)
	    {
		break;
	    }
	    if (errno != EEXIST)
		throw runtime_error_with_errno("create(qgroup rescan state) failed", errno);
	}
	if (temporary_fd < 0)
	    throw std::runtime_error("could not allocate temporary qgroup rescan state file");
	FileDescriptor temporaryfd(temporary_fd);

	try
	{
	    check_owner_and_mode(temporaryfd.get(), false);
	    ssize_t written = 0;
	    while (written < static_cast<ssize_t>(updated.size()))
	    {
		ssize_t result = write(temporaryfd.get(), updated.data() + written,
				       updated.size() - written);
		if (result < 0 && errno == EINTR)
		    continue;
		if (result <= 0)
		    throw runtime_error_with_errno("write(qgroup rescan state) failed",
						   result < 0 ? errno : EIO);
		written += result;
	    }

	    if (fsync(temporaryfd.get()) < 0)
		throw runtime_error_with_errno("fsync(qgroup rescan state) failed", errno);
	    if (renameat(dirfd.get(), temporary_name.c_str(),
			 dirfd.get(), filesystem_uuid.c_str()) < 0)
		throw runtime_error_with_errno("rename(qgroup rescan state) failed", errno);
	}
	catch (...)
	{
	    unlinkat(dirfd.get(), temporary_name.c_str(), 0);
	    throw;
	}

	if (fsync(dirfd.get()) < 0)
	    throw runtime_error_with_errno("fsync(qgroup rescan directory) failed", errno);

	next_allowed = next_window_time(next_cooldown(reservation_time), window);
	return true;
    }


    bool
    QGroupRescanLimiter::hold_active(const std::string& hold_file, time_t now,
					 time_t& hold_until)
    {
	    hold_until = 0;
	    if (hold_file.empty())
		return false;
	    if (hold_file[0] != '/')
		throw std::runtime_error("qgroup rescan hold file must be an absolute path");
	    if (now < 0)
		throw std::runtime_error("invalid time for qgroup rescan hold");

	    FileDescriptor holdfd(open(hold_file.c_str(), O_RDONLY | O_CLOEXEC |
					   O_NOFOLLOW | O_NONBLOCK));
	    if (holdfd.get() < 0)
	    {
		if (errno == ENOENT)
		    return false;
		throw runtime_error_with_errno("open(qgroup rescan hold) failed", errno);
	    }

	    struct stat st;
	    if (fstat(holdfd.get(), &st) < 0)
		throw runtime_error_with_errno("fstat(qgroup rescan hold) failed", errno);
	    if (!S_ISREG(st.st_mode) || (st.st_mode & 0022) != 0)
		throw std::runtime_error("unsafe qgroup rescan hold file type or mode");

	    char contents[64] = {};
	    const ssize_t bytes = pread(holdfd.get(), contents, sizeof(contents), 0);
	    if (bytes < 0)
		throw runtime_error_with_errno("read(qgroup rescan hold) failed", errno);
	    if (bytes == 0 || bytes == static_cast<ssize_t>(sizeof(contents)))
		throw std::runtime_error("invalid qgroup rescan hold deadline");

	    std::istringstream input(std::string(contents, bytes));
	    int64_t parsed = 0;
	    if (!(input >> parsed) || parsed < 0 ||
		parsed > static_cast<int64_t>(std::numeric_limits<time_t>::max()))
		throw std::runtime_error("invalid qgroup rescan hold deadline");
	    input >> std::ws;
	    if (!input.eof())
		throw std::runtime_error("invalid qgroup rescan hold deadline");

	    hold_until = static_cast<time_t>(parsed);
	    return hold_until > now;
    }


    bool
    shared_policy_matches(const std::vector<QGroupRescanPolicy>& policies,
			  const std::string& filesystem_uuid,
			  const std::string& daily_window,
			  const std::string& hold_file)
    {
	bool found = false;
	for (const QGroupRescanPolicy& policy : policies)
	{
	    if (policy.filesystem_uuid != filesystem_uuid)
		continue;
	    found = true;
	    if (policy.daily_window != daily_window || policy.hold_file != hold_file)
		return false;
	}
	return found;
    }


    void
    QGroupRescanLimiter::validate_window(const std::string& daily_window)
    {
	    (void)parse_window(daily_window);
    }

}
