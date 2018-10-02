/*
 * Copyright (c) [2004-2014] Novell, Inc.
 * Copyright (c) [2016,2018] SUSE LLC
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


#include <string>


namespace snapper
{

    const unsigned long long B = 1;
    const unsigned long long KiB = 1024 * B;
    const unsigned long long MiB = 1024 * KiB;
    const unsigned long long GiB = 1024 * MiB;
    const unsigned long long TiB = 1024 * GiB;
    const unsigned long long PiB = 1024 * TiB;
    const unsigned long long EiB = 1024 * PiB;


    /**
     * Return number of suffixes.
     *
     * @return number of suffixes
     */
    int num_suffixes();


    /**
     * Return a suffix.
     *
     * @param i index of suffix
     * @param classic use classic locale instead of global C++ locale
     * @return suffix
     */
    std::string get_suffix(int i);


    /**
     * Return a pretty description of a size with required precision and using
     * B, KiB, MiB, GiB, TiB, PiB or EiB as unit as appropriate. Supported
     * range is 0 B to (16 EiB - 1 B).
     *
     * @param size size in bytes
     * @param precision number of fraction digits in output
     * @return formatted string
     */
    std::string byte_to_humanstring(unsigned long long size, int precision);

}
