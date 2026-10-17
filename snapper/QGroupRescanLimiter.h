/*
 * Copyright (c) 2026 SUSE LLC
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 */

#ifndef SNAPPER_QGROUP_RESCAN_LIMITER_H
#define SNAPPER_QGROUP_RESCAN_LIMITER_H

#include <ctime>
#include <string>

namespace snapper
{

    class QGroupRescanLimiter
    {
    public:
	static constexpr time_t interval = 24 * 60 * 60;

	explicit QGroupRescanLimiter(const std::string& state_dir =
				     "/var/lib/snapper-qgroup-rescans");

	// Reserve the filesystem's next automatic rescan while serializing other
	// Snapper processes. The daily_window is local time in HH:MM-HH:MM form;
	// the end is exclusive, and a window may cross midnight. When active_scan
	// is true, refresh the reservation even if a cooldown already exists or
	// the observed scan is outside the configured window. The recorded time is
	// rounded up by one second because callers may pass time_t values that have
	// discarded fractional seconds.
	bool reserve(const std::string& filesystem_uuid, time_t now,
		     time_t& next_allowed,
		     const std::string& daily_window = "00:00-24:00",
		     bool active_scan = false) const;

	static void validate_window(const std::string& daily_window);

    private:
	std::string state_dir;
    };

}

#endif
