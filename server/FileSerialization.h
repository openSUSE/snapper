/*
 * Copyright (c) [2015] Red Hat, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef SNAPPER_FILE_SERIALIZATION_H
#define SNAPPER_FILE_SERIALIZATION_H

#include <boost/serialization/split_free.hpp>

#include "snapper/File.h"

namespace boost
{
    namespace serialization
    {
	template<class Archive>
	void save(Archive& ar, const snapper::File& f, const unsigned int version)
	{
	    const unsigned int tmp = f.getPreToPostStatus();
	    ar << f.getName() << tmp;
	}


	template<class Archive>
	void serialize(Archive& ar, snapper::File& f, const unsigned int version)
	{
	    boost::serialization::split_free(ar, f, version);
	}
    }
}

#endif
