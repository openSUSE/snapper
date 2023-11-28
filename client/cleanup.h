/*
 * Copyright (c) [2011-2012] Novell, Inc.
 * Copyright (c) [2016-2020] SUSE LLC
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


#include <functional>

#include "proxy.h"


/*
 * The following three functions do the cleanup based on the conditionals defined in the
 * config, that are hard limit, quota and free space.
 */

void
do_cleanup_number(ProxySnapper* snapper, bool verbose, Plugins::Report& report);

void
do_cleanup_timeline(ProxySnapper* snapper, bool verbose, Plugins::Report& report);

void
do_cleanup_empty_pre_post(ProxySnapper* snapper, bool verbose, Plugins::Report& report);


/*
 * The following three functions do the cleanup only based on the provided
 * conditional. The lower range and min-age defined in the config are respected.
 */

void
do_cleanup_number(ProxySnapper* snapper, bool verbose, std::function<bool()> condition,
		  Plugins::Report& report);

void
do_cleanup_timeline(ProxySnapper* snapper, bool verbose, std::function<bool()> condition,
		    Plugins::Report& report);

void
do_cleanup_empty_pre_post(ProxySnapper* snapper, bool verbose, std::function<bool()> condition,
			  Plugins::Report& report);
