/*
 * Copyright (c) [2011-2014] Novell, Inc.
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


#ifndef SNAPPER_BTRFS_UTILS_H
#define SNAPPER_BTRFS_UTILS_H


#include <string>


namespace snapper
{
    using std::string;


    bool is_subvolume(const struct stat& stat);

    bool create_subvolume(int fddst, const string& name);
    bool create_snapshot(int fd, int fddst, const string& name);
    bool delete_subvolume(int fd, const string& name);

    string get_subvolume(int fd, unsigned long long id);
    unsigned long long get_id(int fd);

}


#endif
