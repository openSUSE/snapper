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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "snapper/Acls.h"
#include "snapper/AppUtil.h"
#include "snapper/Exception.h"
#include "snapper/Log.h"

namespace snapper
{
    bool
    is_acl_signature(const std::string& name)
    {
	for (std::vector<string>::const_iterator cit = _acl_signatures.begin(); cit != _acl_signatures.end(); cit++)
	{
	    if (name == *cit)
		return true;
	}
	return false;
    }

    Acls::Acls(const string& path)
    : allowed_types(0x0), acl_access(NULL), acl_default(NULL)
    {
	struct stat buf;

	int fd = ::open(path.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_NOATIME |
			  O_CLOEXEC);
	if (fd < 0)
	{
	    if (errno == ELOOP)
	    {
		y2deb("can't read ACLs from symlink '" << path << "' itself");
		return;
	    }

	    if (stat(path.c_str(), &buf) < 0)
	    {
		y2err("stat failed errno: " << errno << " (" << stringerror(errno) << ")");
		throw AclException();
	    }
	}
	else
	{
	    if (fstat(fd, &buf) < 0)
	    {
		y2err("fstat failed errno: " << errno << " (" << stringerror(errno) << ")");
		::close(fd);
		throw AclException();
	    }

	    acl_access = acl_get_fd(fd);
	    if (!acl_access)
	    {
		y2err("acl_get_fd failed errno: " << errno << " (" << stringerror(errno) << ")");
		::close(fd);
		throw AclException();
	    }

	    ::close(fd);
	    allowed_types = ACL_TYPE_ACCESS;
	}

	allowed_types |= (S_ISDIR(buf.st_mode)) ? ACL_TYPE_DEFAULT : 0x0;

	// in case open failed for some reason
	if (!(allowed_types & ACL_TYPE_ACCESS))
	{
	    allowed_types |= ACL_TYPE_ACCESS;
	    acl_access = acl_get_file(path.c_str(), ACL_TYPE_ACCESS);
	    if (!acl_access)
	    {
		y2err("acl_get_file failed errno: " << errno << " (" << stringerror(errno) << ")");
		throw AclException();
	    }
	}

	// ACL_TYPE_DEFAULT can't be read from fd
	if (allowed_types & ACL_TYPE_DEFAULT)
	{
	    acl_default = acl_get_file(path.c_str(), ACL_TYPE_DEFAULT);
	    if (!acl_default)
	    {
		y2err("acl_get_file failed errno: " << errno << " (" << stringerror(errno) << ")");
		if (acl_free(acl_access))
		{
		    y2err("acl_free failed errno: " << errno << " (" << stringerror(errno) << ")");
		}

		throw AclException();
	    }
	}
    }


    Acls::~Acls()
    {
	if (acl_access)
	    acl_free(acl_access);
	if (acl_default)
	    acl_free(acl_default);
    }


    void
    Acls::serializeTo(const string& path) const
    {
	if (empty())
	    return;

	if (acl_set_file(path.c_str(), ACL_TYPE_ACCESS, acl_access))
	{
	    y2err("acl_set_file failed errno: " << errno << " (" << stringerror(errno) << ")");
	    throw AclException();
	}

	if (get_acl_types() & ACL_TYPE_DEFAULT)
	{
	    if (acl_set_file(path.c_str(), ACL_TYPE_DEFAULT, acl_default))
	    {
		y2err("acl_set_file failed errno: " << errno << " (" << stringerror(errno) << ")");
		throw AclException();
	    }
	}
    }
}
