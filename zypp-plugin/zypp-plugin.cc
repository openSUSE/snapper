/*
 * Copyright (c) [2019-2020] SUSE LLC
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



using namespace std;


#include "zypp-plugin.h"


int
ZyppPlugin::main()
{
    while (true)
    {
	Message msg = read_message(pin);
	if (pin.eof())
	    break;

	Message reply = dispatch(msg);
	write_message(pout, reply);
    }

    return 0;
}


ZyppPlugin::Message
ZyppPlugin::dispatch(const Message& msg)
{
    if (msg.command == "_DISCONNECT")
	return ack();

    Message a;
    a.command = "_ENOMETHOD";
    a.headers["Command"] = msg.command;
    return a;
}
