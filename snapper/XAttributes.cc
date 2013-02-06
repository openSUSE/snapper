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

#include "config.h"
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
#include "snapper/XAttributes.h"

#include <boost/scoped_array.hpp>

namespace snapper
{
    struct XAModification::LinkSetHelper {
        const string *plink;
        LinkSetHelper(const string &link) : plink(&link) {}

        int operator()(const string&, const xa_value_t&, int);
    };

    int
    XAModification::LinkSetHelper::operator()(const string &name, const xa_value_t &value, int flags)
    {
        // TODO: add logging
        if (value.empty())
            return lsetxattr(plink->c_str(), name.c_str(), NULL, 0, flags);
        else
            return lsetxattr(plink->c_str(), name.c_str(), (void *) &value.front(), value.size(), flags);
    }

    struct XAModification::LinkRmHelper {
        const string *plink;
        LinkRmHelper(const string &link) : plink(&link) {}

        int operator()(const string &name) { return lremovexattr(plink->c_str(), name.c_str()); }
    };

    struct XAModification::FileSetHelper {
        const int fd;
        FileSetHelper(int fd) : fd(fd) {}

        int operator()(const string&, const xa_value_t&, int);
    };

    int
    XAModification::FileSetHelper::operator()(const string &name, const xa_value_t &value, int flags)
    {
        // TODO: add logging
        if (value.empty())
            return fsetxattr(fd, name.c_str(), NULL, 0, flags);
        else
            return fsetxattr(fd, name.c_str(), (void *) &value.front(), value.size(), flags);
    }

    struct XAModification::FileRmHelper {
        const int fd;
        FileRmHelper(int fd) : fd(fd) {}

        int operator()(const string &name) { return fremovexattr(fd, name.c_str()); }
    };


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

    XAttributes::XAttributes(const string &linkpath)
    {
        y2deb("entering Xattributes(string link) constructor");
        ssize_t size = llistxattr(linkpath.c_str(), NULL, 0);
        if (size < 0)
        {
            y2err("Couldn't get xattributes names-list size. link: " << linkpath << ", error: " << stringerror(errno));
            throw XAttributesException();
        }

        // +1 to cover size == 0
        boost::scoped_array<char> names(new char[size + 1]);
        names[size] = '\0';

        y2deb("XAttributes names-list size is: " << size);

        size = llistxattr(linkpath.c_str(), names.get(), size);
        if (size < 0)
        {
            y2err("Couldn't get xattributes names-list. link: " << linkpath << ", error: " << stringerror(errno));
            throw XAttributesException();
        }

        int pos = 0;

        while (pos < size)
        {
            string name = string(names.get() + pos);
            // move beyond separating '\0' char
            pos += name.length() + 1;

            ssize_t v_size = lgetxattr(linkpath.c_str(), name.c_str(), NULL, 0);
            if (v_size < 0)
            {
                y2err("Couldn't get a xattribute value size for the xattribute name '" << name << "': " << stringerror(errno));
                throw XAttributesException();
            }

            y2deb("XAttribute value size for xattribute name: '" << name << "' is " << v_size);

            boost::scoped_array<uint8_t> buffer(v_size ? new uint8_t[v_size] : NULL);

            v_size = lgetxattr(linkpath.c_str(), name.c_str(), (void *)buffer.get(), v_size);
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
        type = xa.type;
    }

    XAttributes&
    XAttributes::operator=(const XAttributes &xa)
    {
        y2deb("Entering XAttribute::operator=()");
        if (this != &xa)
        {
            this->xamap = xa.xamap;
            type = xa.type;
        }

        return *this;
    }

    bool
    XAttributes::operator==(const XAttributes& xa) const
    {
        y2deb("Entering XAttribute::operator==()");
        // TODO: what about file type?
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
        int pos = 0;

        xa_value_t::const_iterator cit = xavalue.begin();

        while (cit != xavalue.end())
        {
            sprintf(tmp, "%d", *cit);
            out << '<' << pos++ << '>' << tmp;
            if (++cit != xavalue.end())
                out << ':';
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
        if (src_xa.getType() != dest_xa.getType())
            throw XAttributesException();

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
        for (xa_mod_citer cit = this->cbegin(); cit != this->cend(); cit++)
            if (!cit->second.empty())
                return false;

        return true;
    }

    bool
    XAModification::serializeTo(xattr_set_cb_t set_xattr, xattr_rm_cb_t rm_xattr) const
    {
        for (xa_mod_citer cit = this->cbegin(); cit != this->cend(); cit++)
        {
            for (xa_mod_vec_citer mod_cit = cit->second.begin(); mod_cit != cit->second.end(); mod_cit++)
            {
                switch (cit->first)
                {
                    case XA_DELETE:
                        y2deb("delete xattribute: " << mod_cit->first);
                        if (rm_xattr(mod_cit->first))
                        {
                            y2err("Couldn't remove xattribute '" << mod_cit->first << "': " << stringerror(errno));
                            return false;
                        }
                        break;

                    case XA_REPLACE:
                        y2deb("replace xattribute: " << mod_cit->first);

                        if (set_xattr(mod_cit->first, mod_cit->second, XATTR_REPLACE))
                        {
                            y2err("Couldn't replace xattribute '" << mod_cit->first << "'");
                            y2deb("replace value was: " << mod_cit->second);
                            return false;
                        }
                        break;

                    case XA_CREATE:
                        y2deb("create xattribute: " << mod_cit->first);

                        if (set_xattr(mod_cit->first, mod_cit->second, XATTR_CREATE))
                        {
                            y2err("Couldn't create xattribute '" << mod_cit->first << "'");
                            y2deb("new value was: " << mod_cit->second);
                            return false;
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

    bool
    XAModification::serializeToFd(int dest_fd) const
    {
        if (this->isEmpty())
            return true;

        xattr_rm_cb_t rm_cb = FileRmHelper(dest_fd);
        xattr_set_cb_t set_cb = FileSetHelper(dest_fd);

        return serializeTo(set_cb, rm_cb);
    }


    bool
    XAModification::serializeToLink(const string &link) const
    {
        if (this->isEmpty())
            return true;

        xattr_rm_cb_t rm_cb = LinkRmHelper(link);
        xattr_set_cb_t set_cb = LinkSetHelper(link);

        return serializeTo(set_cb, rm_cb);
    }

    ostream&
    operator<<(ostream &out, const XAModification &xa_mod)
    {
        for (xa_mod_vec_citer cit = xa_mod[XA_DELETE].begin(); cit != xa_mod[XA_DELETE].end(); cit++)
            out << "D:" + cit->first << std::endl;
        for (xa_mod_vec_citer cit = xa_mod[XA_REPLACE].begin(); cit != xa_mod[XA_REPLACE].end(); cit++)
            out << "M:" + cit->first << std::endl;
        for (xa_mod_vec_citer cit = xa_mod[XA_CREATE].begin(); cit != xa_mod[XA_CREATE].end(); cit++)
            out << "C:" + cit->first << std::endl;

        return out;
    }
}