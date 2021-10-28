/*
 * Copyright (c) 2020 SUSE LLC
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


// This tiny file is separated from AppUtil.cc to allow setting specific
// defines. Unsetting _GNU_SOURCE in AppUtil.cc causes many errors. See
// https://github.com/openSUSE/snapper/pull/581.

// Defines to get the XSI-compliant strerror_r.
#define _POSIX_C_SOURCE 200809L
#undef _GNU_SOURCE

#include <string.h>
#include <string>


namespace snapper
{
    using namespace std;


    string
    stringerror(int errnum)
    {
	char buf[128];

	// The assignment to int is a safety net that breaks with the GNU version of
	// strerror_r (which returns char*).

	int r = strerror_r(errnum, buf, sizeof(buf) - 1);
	if (r != 0)
	    return string("strerror_r failed");

	return string(buf);
    }

}
