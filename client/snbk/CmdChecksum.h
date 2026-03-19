/*
 * Copyright (c) 2026 SUSE LLC
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


#ifndef SNAPPER_CMD_CHECKSUM_H
#define SNAPPER_CMD_CHECKSUM_H


#include "Shell.h"


namespace snapper
{
    using std::string;


    /**
     * Get the checksum (e.g. sha256sum) of the file at the given path.
     */
    class CmdChecksum
    {
    public:

	CmdChecksum(const Shell& shell, const string& checksum_bin, const string& path);

	const string& get_checksum() const { return checksum; }

	friend std::ostream& operator<<(std::ostream& s, const CmdChecksum& cmd_checksum);

    private:

	void parse(const vector<string>& lines);

	const string path;
	string checksum;

    };


} // namespace snapper


#endif
