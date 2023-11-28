/*
 * Copyright (c) [2013-2014] Red Hat, Inc.
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


#include "config.h"

#include <sys/xattr.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <iomanip>
#include <boost/scoped_array.hpp>
#include <algorithm>

#include "snapper/AppUtil.h"
#include "snapper/Exception.h"
#include "snapper/Log.h"
#include "snapper/XAttributes.h"
#include "snapper/Acls.h"
#include "snapper/SnapperTmpl.h"


namespace snapper
{
    struct FilterAclsHelper
    {
	FilterAclsHelper(const vector<string>& acl_sigs)
	    : acl_sigs(acl_sigs) {}

	bool operator()(const xa_pair_t& pair)
	{
	    return contains(acl_sigs, pair.first);
	}

	bool operator()(const string& name)
	{
	    return contains(acl_sigs, name);
	}

	const vector<string>& acl_sigs;
    };


    struct InsertAclsHelper
    {
	InsertAclsHelper(xa_map_t& xamap, const vector<string>& acl_sigs)
	: map(xamap), acl_sigs(acl_sigs) {}
	void operator()(const xa_pair_t& xapair)
	{
	    if (contains(acl_sigs, xapair.first))
		map.insert(xapair);
	}

	xa_map_t& map;
	const vector<string>& acl_sigs;
    };


    XAttributes::XAttributes(const string &path)
    {
        y2deb("entering Xattributes(path=" << path << ") constructor");

        ssize_t size = llistxattr(path.c_str(), NULL, 0);
        if (size < 0)
        {
            y2err("Couldn't get xattributes names-list size. link: " << path
		  << ", error: " << stringerror(errno));
            SN_THROW(XAttributesException());
        }

        y2deb("XAttributes names-list size is: " << size);

	if (size == 0)
	    return;

	boost::scoped_array<char> names(new char[size]);

        size = llistxattr(path.c_str(), names.get(), size);
        if (size < 0)
        {
            y2err("Couldn't get xattributes names-list. link: " << path <<
		  ", error: " << stringerror(errno));
            SN_THROW(XAttributesException());
        }

	ssize_t pos = 0;
        while (pos < size)
        {
            string name = string(names.get() + pos);
            // move beyond separating '\0' char
            pos += name.length() + 1;


            ssize_t v_size = lgetxattr(path.c_str(), name.c_str(), NULL, 0);
            if (v_size < 0)
            {
                y2err("Couldn't get a xattribute value size for the xattribute name '" << name
		      << "': " << stringerror(errno));
                SN_THROW(XAttributesException());
            }

            y2deb("XAttribute value size for xattribute name: '" << name << "' is " << v_size);

	    boost::scoped_array<uint8_t> buffer(new uint8_t[v_size]);

	    if (v_size > 0)
	    {
		v_size = lgetxattr(path.c_str(), name.c_str(), buffer.get(), v_size);
		if (v_size < 0)
		{
		    y2err("Couldn't get xattribute value for the xattribute name '" << name << "': ");
		    SN_THROW(XAttributesException());
		}
	    }

	    if (!xamap.insert(make_pair(name, xa_value_t(buffer.get(), buffer.get() + v_size))).second)
	    {
		y2err("Duplicite extended attribute name in source file!");
		SN_THROW(XAttributesException());
	    }
	}

	assert(pos == size);
    }


    XAttributes::XAttributes(const SFile& file)
    {
	y2deb("entering Xattributes(path=" << file.fullname(true) << ") constructor");

	ssize_t size = file.listxattr(NULL, 0);
	if (size < 0)
	{
	    y2err("Couldn't get xattributes names-list size. link: " << file.fullname(true) <<
		  ", error: " << stringerror(errno));
	    SN_THROW(XAttributesException());
	}

	y2deb("XAttributes names-list size is: " << size);

	if (size == 0)
	    return;

	boost::scoped_array<char> names(new char[size]);

	size = file.listxattr(names.get(), size);
	if (size < 0)
	{
	    y2err("Couldn't get xattributes names-list. link: " << file.fullname(true) <<
		  ", error: " << stringerror(errno));
	    SN_THROW(XAttributesException());
	}

	ssize_t pos = 0;
	while (pos < size)
	{
	    string name = string(names.get() + pos);
	    // move beyond separating '\0' char
	    pos += name.length() + 1;

	    ssize_t v_size = file.getxattr(name.c_str(), NULL, 0);
	    if (v_size < 0)
	    {
		y2err("Couldn't get a xattribute value size for the xattribute name '" <<
		      name << "': " << stringerror(errno));
		SN_THROW(XAttributesException());
	    }

	    y2deb("XAttribute value size for xattribute name: '" << name << "' is " << v_size);

	    boost::scoped_array<uint8_t> buffer(new uint8_t[v_size]);

	    if (v_size > 0)
	    {
		v_size = file.getxattr(name.c_str(), buffer.get(), v_size);
		if (v_size < 0)
		{
		    y2err("Couldn't get xattribute value for the xattribute name '" << name << "': ");
		    SN_THROW(XAttributesException());
		}
	    }

	    if (!xamap.insert(make_pair(name, xa_value_t(buffer.get(), buffer.get() + v_size))).second)
	    {
		y2err("Duplicite extended attribute name in source file!");
		SN_THROW(XAttributesException());
	    }
	}

	assert(pos == size);
    }


    bool
    XAttributes::operator==(const XAttributes& xa) const
    {
        y2deb("Entering XAttribute::operator==()");
        return (this == &xa) ? true : (this->xamap == xa.xamap);
    }


    ostream&
    operator<<(ostream &out, const XAttributes &xa)
    {
        xa_map_citer cit = xa.cbegin();

        if (cit == xa.cend())
            out << "(XA container is empty)";

        for (; cit != xa.cend(); cit++)
            out << "xa_name: " << cit->first << ", xa_value: " << cit->second << std::endl;

        return out;
    }


    ostream&
    operator<<(ostream &out, const xa_value_t &xavalue)
    {
        int pos = 0;

        xa_value_t::const_iterator cit = xavalue.begin();

        while (cit != xavalue.end())
        {
            out << '<' << pos++ << '>' << static_cast<int>(*cit);
            if (++cit != xavalue.end())
                out << ':';
        }

        return out;
    }


    XAModification::XAModification(const XAttributes& src_xa, const XAttributes& dest_xa)
    {
        xa_map_citer src_cit = src_xa.cbegin();
        xa_map_citer dest_cit = dest_xa.cbegin();

        while (src_cit != src_xa.cend() && dest_cit != dest_xa.cend())
        {
            y2deb("src_xa_name: " << src_cit->first);
            y2deb("dest_xa_name: " << dest_cit->first);

            if (src_cit->first == dest_cit->first)
            {
                if (src_cit->second != dest_cit->second)
                {
                    y2deb("adding replace operation for " << src_cit->first);
                    replace_vec.push_back(xa_pair_t(src_cit->first, src_cit->second));
                }

                src_cit++;
                dest_cit++;
            }
            else if (src_cit->first < dest_cit->first)
            {
                y2deb("src name < dest name");
		y2deb("adding create operation for " << src_cit->first);
                create_vec.push_back(xa_pair_t(src_cit->first, src_cit->second));

                src_cit++;
            }
            else
            {
                y2deb("src name > dest name");
		y2deb("adding delete operation for " << dest_cit->first);
		delete_vec.push_back(dest_cit->first);

                dest_cit++;
            }
        }

        for (; dest_cit != dest_xa.cend(); dest_cit++)
	{
	    y2deb("adding delete operation for " << dest_cit->first);
	    delete_vec.push_back(dest_cit->first);
	}

        for (; src_cit != src_xa.cend(); src_cit++)
	{
	    y2deb("adding create operation for " << src_cit->first);
            create_vec.push_back(xa_pair_t(src_cit->first, src_cit->second));
	}

    }


    bool
    XAModification::empty() const
    {
	return create_vec.empty() && delete_vec.empty() && replace_vec.empty();
    }


    bool
    XAModification::serializeTo(const string &dest) const
    {
        if (empty())
            return true;

	for (xa_mod_vec_citer cit = create_vec.begin(); cit != create_vec.end(); cit++)
	{
	    y2deb("Create xattribute: " << cit->first);
	    if (cit->second.empty())
	    {
		y2deb("New value for xattribute is empty!");
		if (lsetxattr(dest.c_str(), cit->first.c_str(), NULL, 0, XATTR_CREATE))
		{
		    y2err("Create xattribute with empty value failed: " << stringerror(errno));
		    return false;
		}
	    }
	    else
	    {
		y2deb("New value for xattribute: " << cit->second);
		if (lsetxattr(dest.c_str(), cit->first.c_str(), &cit->second.front(), cit->second.size(), XATTR_CREATE))
		{
		    y2err("Create xattribute '" << cit->first << "' failed: " << stringerror(errno));
		    return false;
		}
	    }
	}

	for (xa_del_vec_citer dcit = delete_vec.begin(); dcit != delete_vec.end(); dcit++)
	{
	    y2deb("Remove xattribute: " << *dcit);
	    if (lremovexattr(dest.c_str(), dcit->c_str()))
	    {
		y2err("Remove xattribute '" << *dcit << "' failed: " << stringerror(errno));
		return false;
	    }
	}

	for (xa_mod_vec_citer rcit = replace_vec.begin(); rcit != replace_vec.end(); rcit++)
	{
	    y2deb("Replace xattribute: " << rcit->first);

	    if (rcit->second.empty())
	    {
		y2deb("new value for xattribute is empty!");
		if (lsetxattr(dest.c_str(), rcit->first.c_str(), NULL, 0, XATTR_REPLACE))
		{
		    y2err("Replace xattribute '" << rcit->first << "' by new (empty) value failed: " << stringerror(errno));
		    return false;
		}
	    }
	    else
	    {
		y2deb("new value for xattribute: " << rcit->second);
		if (lsetxattr(dest.c_str(), rcit->first.c_str(), &rcit->second.front(), rcit->second.size(), XATTR_REPLACE))
		{
		    y2err("Replace xattribute '" << rcit->first << "' by new value failed: " << stringerror(errno));
		    return false;
		}
	    }
	}

	return true;
    }


    unsigned int
    XAModification::getXaCreateNum() const
    {
        return create_vec.size();
    }


    unsigned int
    XAModification::getXaDeleteNum() const
    {
        return delete_vec.size();
    }


    unsigned int
    XAModification::getXaReplaceNum() const
    {
        return replace_vec.size();
    }


    void
    XAModification::printTo(ostream& out, bool diff) const
    {
	char create_lbl = '+', delete_lbl = '-';

	if (diff)
	{
	    create_lbl = delete_lbl;
	    delete_lbl = '+';
	}

	for (xa_del_vec_citer dcit = delete_vec.begin(); dcit != delete_vec.end(); dcit++)
	{
	    out << std::setw(3) << std::right << delete_lbl << ':' << *dcit << std::endl;
	}

        for (xa_mod_vec_citer cit = replace_vec.begin(); cit != replace_vec.end(); cit++)
	{
	    out << std::setw(3) << std::right << "-+" << ':' << cit->first << std::endl;
	}

        for (xa_mod_vec_citer cit = create_vec.begin(); cit != create_vec.end(); cit++)
	{
	    out << std::setw(3) << std::right << create_lbl << ':' << cit->first << std::endl;
	}
    }


    void
    XAModification::dumpDiffReport(ostream& out) const
    {
	printTo(out, true);
    }


    ostream&
    operator<<(ostream &out, const XAModification &xa_mod)
    {
	xa_mod.printTo(out, false);
        return out;
    }


    CompareAcls::CompareAcls(const XAttributes& xa)
    {
	std::for_each(xa.cbegin(), xa.cend(), InsertAclsHelper(xamap, _acl_signatures));
    }


    bool
    CompareAcls::operator==(const CompareAcls& acls) const
    {
	return (this == &acls) ? true : (this->xamap == acls.xamap);
    }


    void
    XAModification::filterOutAcls()
    {
	FilterAclsHelper fhelper(_acl_signatures);

	create_vec.erase(std::remove_if(create_vec.begin(), create_vec.end(), fhelper),
			 create_vec.end());
	delete_vec.erase(std::remove_if(delete_vec.begin(), delete_vec.end(), fhelper),
			 delete_vec.end());
	replace_vec.erase(std::remove_if(replace_vec.begin(), replace_vec.end(), fhelper),
			  replace_vec.end());
    }

}
