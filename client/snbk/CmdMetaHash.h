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


#ifndef SNAPPER_CMD_META_HASH_H
#define SNAPPER_CMD_META_HASH_H


#include <map>

#include "Shell.h"


namespace snapper
{
    using std::string;


    /**
     * Find the hashes (by checksum) of info.xml for all snapshots.
     */
    class CmdMetaHash
    {
    public:

	CmdMetaHash(const Shell& shell, const string& chksum_bin, const string& sh_bin,
	            const string& path);

	const string& get_hash(unsigned int num) const;

	friend std::ostream& operator<<(std::ostream& s, const CmdMetaHash& cmd_metahash);

    private:

	const string path;

	std::map<unsigned int, string> lookup;

	void parse(const std::vector<string>& lines);
    };

} // namespace snapper


#endif
