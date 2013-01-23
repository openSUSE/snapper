/*
 * Copyright (c) [2013] Red Hat, Inc.
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

#include "XAttributes.h"

#include <cstdio>
#include <attr/xattr.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <attr/xattr.h>
#include <fcntl.h>
#include <unistd.h>

#include "snapper/AppUtil.h"
#include "snapper/Exception.h"
#include "snapper/Log.h"

#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

namespace snapper
{
    XAttributes::XAttributes(int fd)
    {
        y2deb("entering Xattributes(int fd) constructor");
        ssize_t size = flistxattr(fd, NULL, 0);
        if (size < 0)
        {
            y2err("Couldn't get xattributes names-list size: " << stringerror(errno));
            throw IOErrorException();
        }

        // +1 to cover size == 0
        boost::scoped_array<char> names(new char[size + 1]);
        names[size] = '\0';

        y2deb("XAttributes names-list size is: " << size);
        
        size = flistxattr(fd, names.get(), size);
        if (size < 0)
        {
            y2err("Couldn't get xattributes names-list: " << stringerror(errno));
            throw IOErrorException();
        }
        
        int pos = 0;

        while (pos < size)
        {
            string name = string(names.get() + pos);
            // move beyond separating '\0' char
            pos += name.length() + 1;

            ssize_t v_size = fgetxattr(fd, name.c_str(), NULL, 0);
            if (v_size < 0)
            {
                y2err("Couldn't get a xattribute value size for the xattribute name '" << name << "': " << stringerror(errno));
                throw IOErrorException();
            }
            
            y2deb("XAttribute value size for xattribute name: '" << name << "' is " << v_size);

            boost::scoped_array<uint8_t> buffer(v_size ? new uint8_t[v_size] : NULL);

            v_size = fgetxattr(fd, name.c_str(), (void *)buffer.get(), v_size);
            if (v_size < 0)
            {
                y2err("Coudln't get xattrbitue value for the xattrbite name '" << name << "': ");
                throw IOErrorException();
            }

            if (!xamap.insert(xa_pair_t(name, xa_value_t(buffer.get(), buffer.get() + v_size))).second)
            {
                y2err("Duplicite extended attribute name in source file!");
                // TODO: possible XAException candidate
                throw IOErrorException();
            }
        }
    }

    XAttributes::XAttributes(const XAttributes &xa)
    {
        y2deb("Starting copy constructor XAttribute(const XAttribute&)");
        xamap = xa.xamap;
    }
    
    xa_find_pair_t
    XAttributes::find(const string &xa_name) const
    {
        xa_map_citer cit = xamap.find(xa_name);
        if (cit != xamap.end())
        {
            return xa_find_pair_t(true, cit->second);
        }
        
        return xa_find_pair_t(false, xa_value_t());
    }

    bool
    XAttributes::serializeModificationsTo(int dest_fd, const XAModification &xa_mods) const
    {
        if (this != xa_mods.getDestination())
        {
            return false;
            // TODO: throw exception: invalid request
        }

        for (xa_mod_citer cit = xa_mods.cbegin(); cit != xa_mods.cend(); cit++)
        {
            for (xa_name_vec_citer name_cit = cit->second.begin(); name_cit != cit->second.end(); name_cit++)
            {
                switch (cit->first)
                {
                    case XA_DELETE:
                        y2deb("delete xattribute: " << *name_cit);
                        if (fremovexattr(dest_fd, (*name_cit).c_str()))
                        {
                            y2err("Couldn't remove xattribute '" << *name_cit << "': " << stringerror(errno));
                            return false;
                        }
                        break;

                    case XA_REPLACE:
                        y2deb("replace xattribute: " << *name_cit);
                        {
                            xa_find_pair_t fnd = find(*name_cit);
                            if (!fnd.first)
                            {
                                y2err("Internal error: Couldn't find xattribute '" << *name_cit << "'");
                                return false;
                            }
                            if (fnd.second.empty())
                            {
                                y2deb("new value for xattribute '" << *name_cit << "' is empty!");
                                if (fsetxattr(dest_fd, (*name_cit).c_str(), NULL, 0, XATTR_REPLACE))
                                {
                                    y2err("Couldn't replace xattribute '" << *name_cit << "' by new (empty) value: " << stringerror(errno));
                                    return false;
                                }
                            }
                            else
                            {
                                y2deb("new value: '" << fnd.second << "'");
                                if (fsetxattr(dest_fd, (*name_cit).c_str(), &fnd.second.front(), fnd.second.size(), XATTR_REPLACE))
                                {
                                    y2err("Couldn't replace xattribute by new value: " << stringerror(errno));
                                    return false;
                                }
                            }
                        }
                        break;

                    case XA_CREATE:
                        y2deb("create xattribute: " << *name_cit);
                        {
                            xa_find_pair_t fnd = find(*name_cit);
                            if (!fnd.first)
                            {
                                y2err("Internal error: Couldn't find xattribute '" << *name_cit << "'");
                                return false;
                            }
                            if (fnd.second.empty())
                            {
                                y2deb("new value for xattribute '" << *name_cit << "' is empty!");
                                if (fsetxattr(dest_fd, (*name_cit).c_str(), NULL, 0, XATTR_CREATE))
                                {
                                    y2err("Couldn't create xattribute '" << *name_cit << "' by new (empty) value: " << stringerror(errno));
                                    return false;
                                }
                            }
                            else
                            {
                                y2deb("new value: '" << fnd.second << "'");
                                if (fsetxattr(dest_fd, (*name_cit).c_str(), &fnd.second.front(), fnd.second.size(), XATTR_CREATE))
                                {
                                    y2err("Couldn't create xattribute '" << *name_cit << "': " << stringerror(errno));
                                    return false;
                                }
                            }
                        }
                        break;

                    default:
                        y2err("Internal Error in XAttributes()");
                        return false;
                }
            }
        }
        return true;
    }

    XAttributes&
    XAttributes::operator=(const XAttributes &xa)
    {
        y2deb("Entering XAttribute::operator=()");
        if (this != &xa)
        {
            this->xamap = xa.xamap;
        }

        return *this;
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
        char tmp[4];

        for (xa_value_t::const_iterator cit = xavalue.begin(); cit != xavalue.end(); cit++)
        {
            sprintf(tmp, "%d", *cit);
            out << tmp << ":";
        }

        return out;
    }

    ostream&
    operator<<(ostream &out, const xa_modification_t &xa_change)
    {
        if (xa_change.find(XA_DELETE)->second.empty() &&
            xa_change.find(XA_REPLACE)->second.empty() &&
            xa_change.find(XA_CREATE)->second.empty()
           )
            out << "(xa_modification_t is empty)";
        else
        {
            for (xa_mod_citer cit = xa_change.begin(); cit != xa_change.end(); cit++)
            {
                switch (cit->first)
                {
                    case XA_DELETE:
                        out << "XA_DELETE:";
                        break;
                    case XA_REPLACE:
                        out << "XA_REPLACE:";
                        break;
                    case XA_CREATE:
                        out << "XA_CREATE:";
                        break;
                    default:
                        out << "(!!!unknown!!!)";
                }
                for (xa_name_vec_citer name_cit = cit->second.begin(); name_cit != cit->second.end(); name_cit++)
                    out << *name_cit << ',' << std::endl;
            }
        }

        return out;
    }

    XAModification::XAModification()
    {
        xamodmap[XA_DELETE] = xa_name_vec_t();
        xamodmap[XA_REPLACE] = xa_name_vec_t();
        xamodmap[XA_CREATE] = xa_name_vec_t();

        p_xa_dest = NULL;
    }

    XAModification::XAModification(const XAttributes &src_xa, XAttributes &dest_xa)
    {
        xamodmap[XA_DELETE] = xa_name_vec_t();
        xamodmap[XA_REPLACE] = xa_name_vec_t();
        xamodmap[XA_CREATE] = xa_name_vec_t();

        xa_map_citer src_cit = src_xa.xamap.begin();

        boost::scoped_ptr<xa_map_t> p_xamap(new xa_map_t(dest_xa.xamap));

        xa_map_iter it = p_xamap->begin();

        while (src_cit != src_xa.xamap.end() && it != p_xamap->end())
        {
            y2deb("this src_xa_name: " << src_cit->first);
            y2deb("this dest_xa_name: " << it->first);

            if (src_cit->first == it->first)
            {
                y2deb("names matched");
                if (src_cit->second != it->second)
                {
                    y2deb("create XA_REPLACE event");
                    xamodmap[XA_REPLACE].push_back(src_cit->first);
                    (*p_xamap)[it->first] = src_cit->second;
                }
                src_cit++;
                it++;
            }
            else if (src_cit->first < it->first)
            {
                y2deb("src name < dest name");
                xamodmap[XA_CREATE].push_back(src_cit->first);
                it = p_xamap->insert(xa_pair_t(src_cit->first, src_cit->second)).first;
                if (it != p_xamap->end())
                    y2deb("next dest name is " << it->first);

                src_cit++;
            }
            else
            {
                y2deb("src name > dest name");
                xamodmap[XA_DELETE].push_back(it->first);
                xa_map_iter tmp = it++;
                p_xamap->erase(tmp);
                if (it != p_xamap->end())
                    y2deb("next dest name is " << it->first);
            }
        }

        if (it != p_xamap->end())
        {
            xa_map_iter tmp = it;
            while (tmp != p_xamap->end())
            {
                xamodmap[XA_DELETE].push_back(tmp->first);
                tmp++;
            }
            p_xamap->erase(it, p_xamap->end());
        }

        while (src_cit != src_xa.xamap.end())
        {
            xamodmap[XA_CREATE].push_back(src_cit->first);
            if (!p_xamap->insert(xa_pair_t(src_cit->first, src_cit->second)).second)
            {
                // TODO: throw XA error
                y2war("Internal error: XA w/ name: '" << src_cit->first << "' already exists");
            }

            src_cit++;
        }

        // TODO: how to do atomic change in a sane way?
        dest_xa.xamap = *p_xamap;
        this->p_xa_dest = &dest_xa;
    }

    ostream&
    operator<<(ostream &out, const XAModification &xa_mod)
    {
        return out << xa_mod;
    }
}