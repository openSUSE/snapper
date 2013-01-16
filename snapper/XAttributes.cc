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
#include <fcntl.h>
#include <unistd.h>

#include "snapper/Exception.h"

namespace snapper
{
    XAttributes::XAttributes()
    {
        xamap = new xa_map_t;
    }

    XAttributes::XAttributes(int fd)
    {
        // TODO: change to debug
        std::cout << "starting XattrsContainer(int)" << std::endl;
        ssize_t size = flistxattr(fd, NULL, 0);
        if (size < 0)
        {
            std::cerr << "errno: " <<  errno << std::endl;
            throw IOErrorException();
        }

        // +1 to cover size == 0
        char *names = new char[size + 1];

        size = flistxattr(fd, names, size);
        if (size < 0)
        {
            std::cerr << "errno: " <<  errno << std::endl;
            throw IOErrorException();
        }

        // TODO: change to debug
        std::cout << "names list size is: " << size << std::endl;

        int pos = 0;

        xamap = new xa_map_t;

        while (pos < size)
        {
            string name = string(names + pos);
            // step beyond separating '\0' char
            pos += name.length() + 1;

            ssize_t v_size = fgetxattr(fd, name.c_str(), NULL, 0);
            if (v_size < 0)
            {
                std::cerr << "fgetxattr(0) failed w/ errno: " <<  errno << std::endl;
                throw IOErrorException();
            }

            uint8_t *buffer = new uint8_t[v_size];

            v_size = fgetxattr(fd, name.c_str(), buffer, v_size);
            if (v_size < 0)
            {
                std::cerr << "fgetxattr(" << v_size << ") failed w/ errno: " <<  errno << std::endl;
                throw IOErrorException();
            }

            xa_value_t xa_value(buffer, buffer + v_size);

            std::cout << "array size: " << v_size << ", xavalue size: " << xa_value.size() << std::endl;

            xamap->insert(xa_pair_t(name, xa_value));

            delete[] buffer;
        }

        delete[] names;
    }

    XAttributes::XAttributes(const XAttributes &xa)
    {
        std::cout << "starting copy constructor XattrsContainer(const XattrsContainer&)" << std::endl;
        xamap = new xa_map_t(*(xa.xamap));
    }
    
    XAttributes::~XAttributes()
    {
        delete xamap;
    }

    XAttributes&
    XAttributes::operator=(const XAttributes &xa)
    {
        if (this != &xa)
        {
            std::cout << "This is assignment operator XattrsContainer::operator=()" << std::endl;
            delete this->xamap;

            this->xamap = new xa_map_t(*(xa.xamap));
        }

        return *this;
    }

    bool
    XAttributes::operator==(const XAttributes &xa)
    {
        std::cout << "operator==" << std::endl;
        return *(this->xamap) == *(xa.xamap);
    }

    ostream&
    operator<<(ostream &out, const XAttributes &xa)
    {
        xa_map_citer it = xa.xamap->begin();

        if (it == xa.xamap->end())
        {
            out << "(XA container is empty)";
            return out;
        }

        out << "XA container includes:" << std::endl;

        while (it != xa.xamap->end())
        {
            out << "xa name: " << it->first << ", xa value: " << it->second << std::endl;
            it++;
        }

        return out;
    }

    void
    XAttributes::insert(const xa_pair_t &p)
    {
        pair<xa_map_citer, bool> ret = xamap->insert(p);
        if (!ret.second)
            std::cerr << "couldn't insert '" << p.first << ":" << "'" << p.second << "'" << std::endl;

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
}