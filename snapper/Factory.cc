/*
 * Copyright (c) 2011 Novell, Inc.
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


#include <memory>

#include "snapper/Factory.h"
#include "snapper/Snapper.h"
#include "snapper/Exception.h"


namespace snapper
{



    Snapper*
    createSnapper(const string& config_name, bool disable_filters)
    {
        static std::auto_ptr<Snapper> the_one;

	if (the_one.get())
	    throw LogicErrorException();

	the_one.reset(new Snapper(config_name, disable_filters));
	return the_one.get();
    }

/*
    void
    deleteSnapper(Snapper* s)
    {
	if (!the_one.get() || s != the_one.get())
	    throw LogicErrorException();

	the_one.reset();
    }
*/
}
