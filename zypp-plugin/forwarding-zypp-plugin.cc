/*
 * Copyright (c) [2020-2025] SUSE LLC
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

#include <boost/version.hpp>
#if BOOST_VERSION < 108800
#include <boost/process.hpp>
#else
#define BOOST_PROCESS_VERSION 1
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
#endif

#include "zypp-plugin.h"

using namespace std;
namespace bp = boost::process;


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
    bp::ipstream child_out;	// we read this
    bp::opstream child_in;	// we write this
};


ForwardingZyppPlugin::ForwardingZyppPlugin(const string& another_plugin)
    : child_program(another_plugin)
{
}


int
ForwardingZyppPlugin::main()
{
    bp::child child(child_program, bp::std_out > child_out, bp::std_in < child_in);

    return ZyppPlugin::main();
}


ZyppPlugin::Message
ForwardingZyppPlugin::dispatch(const Message& msg)
{
    write_message(child_in, msg);

    return read_message(child_out);
}


int
main(int argc, char** argv)
{
    if (argc != 2)
	throw runtime_error("Usage: forwarding-zypp-plugin ANOTHER_ZYPP_PLUGIN");

    ForwardingZyppPlugin plugin(argv[1]);

    return plugin.main();
}
