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

namespace snapper
{
    XAttributes::XAttributes(int fd)
    {
        y2deb("entering Xattributes(int fd) constructor");
        ssize_t size = flistxattr(fd, NULL, 0);
        if (size < 0)
        {
            y2err("Couldn't get xattributes names-list size: " << stringerror(errno));
            throw XAttributesException();
        }

        // +1 to cover size == 0
        boost::scoped_array<char> names(new char[size + 1]);
        names[size] = '\0';

        y2deb("XAttributes names-list size is: " << size);

        size = flistxattr(fd, names.get(), size);
        if (size < 0)
        {
            y2err("Couldn't get xattributes names-list: " << stringerror(errno));
            throw XAttributesException();
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
                throw XAttributesException();
            }

            y2deb("XAttribute value size for xattribute name: '" << name << "' is " << v_size);

            boost::scoped_array<uint8_t> buffer(v_size ? new uint8_t[v_size] : NULL);

            v_size = fgetxattr(fd, name.c_str(), (void *)buffer.get(), v_size);
            if (v_size < 0)
            {
                y2err("Coudln't get xattrbitue value for the xattrbite name '" << name << "': ");
                throw XAttributesException();
            }

            if (!xamap.insert(xa_pair_t(name, xa_value_t(buffer.get(), buffer.get() + v_size))).second)
            {
                y2err("Duplicite extended attribute name in source file!");
                throw XAttributesException();
            }
        }
    }

    XAttributes::XAttributes(const XAttributes &xa)
    {
        y2deb("Starting copy constructor XAttribute(const XAttribute&)");
        xamap = xa.xamap;
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

        xa_value_t::const_iterator cit = xavalue.begin();

        while (cit != xavalue.end())
        {
            sprintf(tmp, "%d", *cit);
            out << tmp;
            if (++cit == xavalue.end())
                out << "";
            else
                out << ":";
        }

        return out;
    }

    ostream&
    operator<<(ostream &out, const xa_modification_t &xa_change)
    {
        for (xa_mod_citer cit = xa_change.begin(); cit != xa_change.end(); cit++)
        {
            switch (cit->first)
            {
                case XA_DELETE:
                    out << "XA_DELETE:" << std::endl;
                    break;
                case XA_REPLACE:
                    out << "XA_REPLACE:" << std::endl;
                    break;
                case XA_CREATE:
                    out << "XA_CREATE:"  << std::endl;
                    break;
                default:
                    out << "(!!!unknown!!!)";
            }
            for (xa_mod_vec_citer mod_cit = cit->second.begin(); mod_cit != cit->second.end(); mod_cit++)
                out << mod_cit->first << ":'" << mod_cit->second << "'" << std::endl;
        }

        return out;
    }

    XAModification::XAModification()
    {
        xamodmap[XA_DELETE] = xa_mod_vec_t();
        xamodmap[XA_REPLACE] = xa_mod_vec_t();
        xamodmap[XA_CREATE] = xa_mod_vec_t();
    }

    XAModification::XAModification(const snapper::XAttributes& src_xa, const snapper::XAttributes& dest_xa)
    {
        xamodmap[XA_DELETE] = xa_mod_vec_t();
        xamodmap[XA_REPLACE] = xa_mod_vec_t();
        xamodmap[XA_CREATE] = xa_mod_vec_t();

        xa_map_citer src_cit = src_xa.cbegin();
        xa_map_citer dest_cit = dest_xa.cbegin();

        while (src_cit != src_xa.cend() && dest_cit != dest_xa.cend())
        {
            y2deb("this src_xa_name: " << src_cit->first);
            y2deb("this dest_xa_name: " << dest_cit->first);

            if (src_cit->first == dest_cit->first)
            {
                y2deb("names matched");
                if (src_cit->second != dest_cit->second)
                {
                    y2deb("create XA_REPLACE event");
                    xamodmap[XA_REPLACE].push_back(xa_pair_t(src_cit->first, src_cit->second));
                }

                src_cit++;
                dest_cit++;
            }
            else if (src_cit->first < dest_cit->first)
            {
                y2deb("src name < dest name");
                xamodmap[XA_CREATE].push_back(xa_pair_t(src_cit->first, src_cit->second));

                src_cit++;
            }
            else
            {
                y2deb("src name > dest name");
                xamodmap[XA_DELETE].push_back(xa_pair_t(dest_cit->first, xa_value_t()));

                dest_cit++;
            }
        }

        for (; dest_cit != dest_xa.cend(); dest_cit++)
            xamodmap[XA_DELETE].push_back(xa_pair_t(dest_cit->first, xa_value_t()));

        for (; src_cit != src_xa.cend(); src_cit++)
            xamodmap[XA_CREATE].push_back(xa_pair_t(src_cit->first, src_cit->second));
    }

    const xa_mod_vec_t&
    XAModification::operator[](const uint8_t action) const
    {
        return this->xamodmap.find(action)->second;
    }

    bool
    XAModification::isEmpty() const
    {
        return (this->operator[](XA_DELETE).empty() && this->operator[](XA_REPLACE).empty() && this->operator[](XA_CREATE).empty());
    }

    bool
    XAModification::serializeTo(int dest_fd) const
    {
        if (this->isEmpty())
            return true;

        for (xa_mod_citer cit = this->cbegin(); cit != this->cend(); cit++)
        {
            for (xa_mod_vec_citer mod_cit = cit->second.begin(); mod_cit != cit->second.end(); mod_cit++)
            {
                switch (cit->first)
                {
                    case XA_DELETE:
                        y2deb("delete xattribute: " << mod_cit->first);
                        if (fremovexattr(dest_fd, mod_cit->first.c_str()))
                        {
                            y2err("Couldn't remove xattribute '" << mod_cit->first << "': " << stringerror(errno));
                            return false;
                        }
                        break;

                    case XA_REPLACE:
                        y2deb("replace xattribute: " << mod_cit->first);

                        if (mod_cit->second.empty())
                        {
                            y2deb("new value for xattribute '" << mod_cit->first << "' is empty!");
                            if (fsetxattr(dest_fd, mod_cit->first.c_str(), NULL, 0, XATTR_REPLACE))
                            {
                                y2err("Couldn't replace xattribute '" << mod_cit->first << "' by new (empty) value: " << stringerror(errno));
                                return false;
                            }
                        }
                        else
                        {
                            y2deb("new value for xattribute '" << mod_cit->first << "': " << mod_cit->second);
                            if (fsetxattr(dest_fd, mod_cit->first.c_str(), &mod_cit->second.front(), mod_cit->second.size(), XATTR_REPLACE))
                            {
                                y2err("Couldn't replace xattribute '" << mod_cit->first << "' by new (non-empty) value: " << stringerror(errno));
                                y2deb("new XA value size: " << mod_cit->second.size() << ". The value: " << mod_cit->second);
                                return false;
                            }
                        }
                        break;

                    case XA_CREATE:
                        y2deb("create xattribute: " << mod_cit->first);
                        if (mod_cit->second.empty())
                        {
                            y2deb("new value for xattribute '" << mod_cit->first << "' is empty!");
                            if (fsetxattr(dest_fd, mod_cit->first.c_str(), NULL, 0, XATTR_CREATE))
                            {
                                y2err("Couldn't create xattribute '" << mod_cit->first << "' with new (empty) value: " << stringerror(errno));
                                return false;
                            }
                        }
                        else
                        {
                            y2deb("new value for xattribute '" << mod_cit->first << "': " << mod_cit->second);
                            if (fsetxattr(dest_fd, mod_cit->first.c_str(), &mod_cit->second.front(), mod_cit->second.size(), XATTR_CREATE))
                            {
                                y2err("Couldn't create xattribute '" << mod_cit->first << "' with new (non-empty) value: " << stringerror(errno));
                                y2deb("new XA value size: " << mod_cit->second.size() << ". The value: " << mod_cit->second);
                                return false;
                            }
                        }
                        break;

                    default:
                        y2err("Internal Error in XAModification(): unknown action:" << cit->first);
                        return false;
                }
            }
        }
        return true;
    }

    ostream&
    operator<<(ostream &out, const XAModification &xa_mod)
    {
        if (xa_mod.isEmpty())
            out << "(empty)";
        else
            out << xa_mod.xamodmap;

        return out;
    }
}