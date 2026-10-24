/*
 * Copyright (c) 2026 SUSE LLC
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 */

#ifndef SNAPPER_IGNORE_PATTERNS_H
#define SNAPPER_IGNORE_PATTERNS_H


#include <string>
#include <vector>


namespace snapper
{
    /** Read the system-wide ignore patterns. */
    std::vector<std::string>
    readIgnorePatterns();

    namespace detail
    {
        std::vector<std::string>
        readIgnorePatterns(const std::string& etc_filters_dir,
                           const std::string& usr_filters_dir);
    }
}


#endif
