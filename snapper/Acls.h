/*
 * Copyright (c) [2014] Red Hat, Inc.
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

#ifndef SNAPPER_ACLS_H
#define SNAPPER_ACLS_H

#include <string>
#include <vector>
#include <sys/acl.h>

#include <boost/assign/list_of.hpp>

#ifndef ENABLE_ACL_SIGNATURES
#define ENABLE_ACL_SIGNATURES	("system.posix_acl_access") \
				("system.posix_acl_default") \
				("trusted.SGI_ACL_FILE") \
				("trusted.SGI_ACL_DEFAULT")
#endif

namespace snapper
{
    using std::string;

    const std::vector<string> _acl_signatures = boost::assign::list_of ENABLE_ACL_SIGNATURES;

    bool is_acl_signature(const string& name);

    class Acls
    {
    public:

	Acls(const string& path);
	~Acls();

	acl_type_t get_acl_types() const { return allowed_types; }
	bool empty() const { return allowed_types == 0x0; }
	void serializeTo(const string& path) const;

    private:

	acl_type_t allowed_types;
	acl_t acl_access;
	acl_t acl_default;
    };

}
#endif //SNAPPER_ACLS_H
