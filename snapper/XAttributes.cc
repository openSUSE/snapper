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

namespace snapper
{
    XAttributes::XAttributes()
    {
        xamap = new xa_map_t;
        xachmap = NULL;
    }

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
        char *names = new char[size + 1];
        names[size] = '\0';

        y2deb("XAttributes names-list size is: " << size);
        
        size = flistxattr(fd, names, size);
        if (size < 0)
        {
            y2err("Couldn't get xattributes names-list: " << stringerror(errno));
            throw IOErrorException();
        }

        xamap = new xa_map_t;
        
        int pos = 0;

        while (pos < size)
        {
            string name = string(names + pos);
            // move beyond separating '\0' char
            pos += name.length() + 1;

            ssize_t v_size = fgetxattr(fd, name.c_str(), NULL, 0);
            if (v_size < 0)
            {
                y2err("Couldn't get a xattribute value size for the xattribute name '" << name << "': " << stringerror(errno)); 
                throw IOErrorException();
            }
            
            y2deb("XAttribute value size for xattribute name: '" << name << "' is " << size);

            uint8_t *buffer = new uint8_t[v_size];

            v_size = fgetxattr(fd, name.c_str(), (void *)buffer, v_size);
            if (v_size < 0)
            {
                y2err("Coudln't get xattrbitue value for the xattrbite name '" << name << "': ");
                throw IOErrorException();
            }

            xa_value_t xa_value(buffer, buffer + v_size);

            xamap->insert(xa_pair_t(name, xa_value));

            delete[] buffer;
        }

        delete[] names;
        
        xachmap = NULL;
    }

    XAttributes::XAttributes(const XAttributes &xa)
    {
        y2deb("Starting copy constructor XAttribute(const XAttribute&)");
        xamap = new xa_map_t(*(xa.xamap));

        if (xa.xachmap)
            xachmap = new xa_change_t(*(xa.xachmap));
    }
    
    XAttributes::~XAttributes()
    {
        delete xamap;
        delete xachmap;
    }

    xa_find_pair_t
    XAttributes::find(const string &xa_name) const
    {
        xa_map_citer cit = xamap->find(xa_name);
        if (cit != xamap->end())
        {
            return xa_find_pair_t(true, cit->second);
        }
        
        return xa_find_pair_t(false, xa_value_t());
    }
    
    void
    XAttributes::insert(const xa_pair_t &p)
    {
        pair<xa_map_citer, bool> ret = xamap->insert(p);
        if (!ret.second)
            y2war("Couldn't insert pair xa_name==" << p.first << ":xa_value==" << p.second);

    }
    
    void
    XAttributes::generateXaComparison(const XAttributes &src_xa)
    {
        xa_change_t *change_map = new xa_change_t();
        (*change_map)[XA_DELETE] = xa_name_vec_t();
        (*change_map)[XA_REPLACE] = xa_name_vec_t();
        (*change_map)[XA_CREATE] = xa_name_vec_t();

        delete this->xachmap;

        if (*this == src_xa)
        {
            this->xachmap = change_map;

            return;
        }

        xa_map_citer src_cit = src_xa.xamap->begin();
        xa_map_iter it = this->xamap->begin();

        while (src_cit != src_xa.xamap->end() && it != this->xamap->end())
        {
            if (src_cit->first == it->first)
            {
                if (src_cit->second != it->second)
                {
                    (*change_map)[XA_REPLACE].push_back(src_cit->first);
                    (*this->xamap)[src_cit->first] = src_cit->second;
                }
                src_cit++;
                it++;
            }
            else if (src_cit->first < it->first)
            {
                (*change_map)[XA_CREATE].push_back(src_cit->first);
                it = this->xamap->insert(xa_pair_t(src_cit->first, src_cit->second)).first;

                src_cit++;
            }
            else
            {
                (*change_map)[XA_DELETE].push_back(src_cit->first);
                it = this->xamap->erase(it);
            }
        }

        while (src_cit != src_xa.xamap->end())
        {
            (*change_map)[XA_CREATE].push_back(src_cit->first);
            it = this->xamap->insert(xa_pair_t(src_cit->first, src_cit->second)).first;

            src_cit++;
        }

        while (it != this->xamap->end())
        {
            (*change_map)[XA_DELETE].push_back(it->first);
            it = this->xamap->erase(it);
        }

        this->xachmap = change_map;
    }

    bool
    XAttributes::serializeTo(int dest_fd)
    {
        if (!this->xachmap)
        {
            y2war("Missing change map!");
            return false;
        }

        xa_change_citer cit = this->xachmap->begin();

        while(cit != this->xachmap->end())
        {
            xa_name_vec_citer name_cit = cit->second.begin();

            switch (cit->first)
            {
                case XA_DELETE:
                    while (name_cit != cit->second.end())
                    {
                        y2deb("delete xattribute: " << *name_cit);
                        if (fremovexattr(dest_fd, (*name_cit).c_str()))
                        {
                            y2err("Couldn't remove xattribute '" << *name_cit << "': " << stringerror(errno));
                            return false;
                        }
                        name_cit++;
                    }
                    break;

                case XA_REPLACE:
                    while (name_cit != cit->second.end())
                    {
                        y2deb("replace xattribute: " << *name_cit);
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
                        name_cit++;
                    }
                    break;

                case XA_CREATE:
                    while (name_cit != cit->second.end())
                    {
                        y2deb("create xattribute: " << *name_cit);
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
                        name_cit++;
                    }
                    break;

                default:
                    y2err("Internal Error in XAttributes()");
                    return false;
            }
            cit++;
        }
        return true;
    }

    XAttributes&
    XAttributes::operator=(const XAttributes &xa)
    {
        y2deb("Entering XAttribute::operator=()");
        if (this != &xa)
        {
            delete this->xamap;
            this->xamap = new xa_map_t(*(xa.xamap));

            delete this->xachmap;
            this->xachmap = NULL;

            if (xa.xachmap)
            {
                this->xachmap = new xa_change_t(*(xa.xachmap));
            }
        }

        return *this;
    }

    bool
    XAttributes::operator==(const XAttributes& xa) const
    {
        y2deb("Entering XAttribute::operator==()");
        // do not care about change map. the content is xamap only
        return (this == &xa) ? true : (*(this->xamap) == *(xa.xamap));
    }

    ostream&
    operator<<(ostream &out, const XAttributes &xa)
    {
        xa_map_citer it = xa.xamap->begin();

        if (it == xa.xamap->end())
            out << "(XA container is empty)";

        while (it != xa.xamap->end())
        {
            out << "xa_name: " << it->first << ", xa_value: " << it->second << std::endl;
            it++;
        }

        if (xa.xachmap)
        {
            out << "change content: " << std::endl << *xa.xachmap;
        }

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
    operator<<(ostream &out, const xa_change_t &xa_change)
    {
        xa_change_citer cit = xa_change.begin();

        while (cit != xa_change.end())
        {
            xa_name_vec_citer name_cit = cit->second.begin();
            switch (cit->first)
            {
                case XA_DELETE:
                    out << "XA_DELETE";
                    break;
                case XA_REPLACE:
                    out << "XA_REPLACE";
                    break;
                case XA_CREATE:
                    out << "XA_CREATE";
                    break;
                default:
                    out << "unknown";
            }
            out << " mark:" << std::endl;
            while (name_cit != cit->second.end())
                out << *name_cit++ << std::endl;
            cit++;
        }

        return out;
    }
}