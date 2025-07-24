/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) [2016-2025] SUSE LLC
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


#ifndef SNAPPER_CONFIG_INFO_H
#define SNAPPER_CONFIG_INFO_H


#include "snapper/AsciiFile.h"


namespace snapper
{

    class ConfigInfo : public SysconfigFile
    {
    public:

	ConfigInfo(const std::string& config_name, const std::string& root_prefix);

	const std::string& get_config_name() const { return config_name; }
	const std::string& get_subvolume() const { return subvolume; }

	virtual void check_key(const std::string& key) const override;

    private:

	const std::string config_name;
	const std::string root_prefix;

	std::string subvolume;

    };

}


#endif
