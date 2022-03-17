/*
 * Copyright (c) 2020 SUSE LLC
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

#include <string>

using namespace std;

#include <boost/process.hpp>

namespace bp = boost::process;

#include "zypp-plugin.h"


/**
 * A middleware that can wrap an actual plugin for enhanced validation.
 *
 * In a test suite, SnapperZyppPlugin is fed by a shell script. The plugin can
 * validate its input but shell will not validate all aspects of the
 * output. Inserting ForwardingZyppPlugin in between means that ZyppPlugin's
 * validation will check SnapperZyppPlugin's output.
 */
class ForwardingZyppPlugin : public ZyppPlugin
{
public:
    ForwardingZyppPlugin(const string& another_plugin);

    virtual int main() override;
    virtual Message dispatch(const Message&) override;
private:
    string child_program;
    bp::ipstream childs_out; // we read this
    // bp::ipstream childs_err; // we read this
    bp::opstream childs_in; // we write this
};


ForwardingZyppPlugin::ForwardingZyppPlugin(const string& another_plugin)
    : child_program(another_plugin)
{
}


int
ForwardingZyppPlugin::main()
{
    bp::child c(child_program,
		bp::std_out > childs_out,
		// bp::std_err > childs_err,
		bp::std_in < childs_in);

    int result = ZyppPlugin::main();

    //    c.wait();
    return result;
}


ZyppPlugin::Message
ForwardingZyppPlugin::dispatch(const Message& msg)
{
    write_message(childs_in, msg);
    Message reply = read_message(childs_out);
    return reply;
}


int
main(int argc, char** argv)
{
    if (argc != 2)
	throw runtime_error("Usage: forwarding-zypp-plugin ANOTHER_ZYPP_PLUGIN");
    ForwardingZyppPlugin plugin(argv[1]);
    return plugin.main();
}
