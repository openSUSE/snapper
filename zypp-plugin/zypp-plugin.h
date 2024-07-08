/*
 * Copyright (c) 2019 SUSE LLC
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
 * with this program; if not, contact SUSE LLC.
 *
 * To contact SUSE about this file by physical or electronic mail, you may
 * find current contact information at www.suse.com.
 */

#ifndef ZYPP_PLUGIN_H
#define ZYPP_PLUGIN_H


#include <iostream>

#include "../stomp/Stomp.h"


class ZyppPlugin
{
public:

    // Plugin message aka frame
    // https://doc.opensuse.org/projects/libzypp/SLE12SP2/zypp-plugins.html
    using Message = Stomp::Message;

    ZyppPlugin(std::istream& in = std::cin, std::ostream& out = std::cout)
	: pin(in), pout(out)
    {}

    virtual ~ZyppPlugin() = default;

    virtual int main();

protected:

    /// Where the protocol reads from
    std::istream& pin;
    /// Where the protocol writes to
    std::ostream& pout;

    Message read_message(std::istream& is) const { return Stomp::read_message(is); }
    void write_message(std::ostream& os, const Message& msg) const { Stomp::write_message(os, msg); }

    /// Handle a message and return a reply.
    // Derived classes must override it.
    // The base acks a _DISCONNECT and replies _ENOMETHOD to everything else.
    virtual Message dispatch(const Message& msg) = 0;

    Message ack() const { return Stomp::ack(); }

};

#endif
