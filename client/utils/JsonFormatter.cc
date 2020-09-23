/*
 * Copyright (c) [2019-2020] SUSE LLC
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


#include "client/utils/JsonFormatter.h"


namespace snapper
{

    using namespace std;


    ostream&
    operator<<(ostream& stream, const JsonFormatter& json_formatter)
    {
	const int flags = JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_SPACED
#ifdef JSON_C_TO_STRING_NOSLASHESCAPE
	    | JSON_C_TO_STRING_NOSLASHESCAPE
#endif
	    ;

	return stream << json_object_to_json_string_ext(json_formatter._root, flags) << '\n';
    }

}
