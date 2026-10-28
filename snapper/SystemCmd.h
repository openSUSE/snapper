/*
 * Copyright (c) [2004-2014] Novell, Inc.
 * Copyright (c) [2018-2026] SUSE LLC
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
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


#ifndef SNAPPER_SYSTEM_CMD_H
#define SNAPPER_SYSTEM_CMD_H

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <initializer_list>
#include <boost/noncopyable.hpp>


namespace snapper
{
    using std::string;
    using std::string_view;
    using std::vector;


    class SystemCmd final : private boost::noncopyable
    {
    public:


	/**
	 * Mode for starting the process.
	 */
	enum class StartMode
	{
	    SYNC,   // start and wait for completion
	    ASYNC   // start without waiting for completion
	};


	/**
	 * Class holding command with arguments.
	 */
	class Args
	{
	public:

	    Args(std::initializer_list<string> init)
		: values(init) {}

	    const vector<string>& get_values() const { return values; }

	    Args& operator<<(string_view arg) { values.emplace_back(arg); return *this; }

	    Args& operator<<(const vector<string>& args)
		{ values.insert(values.end(), args.begin(), args.end()); return *this; }

	private:

	    vector<string> values;

	};


	SystemCmd(const Args& args, bool log_output = true);

	SystemCmd(const Args& args, StartMode mode, bool log_output = true);

	~SystemCmd();

	/**
	 * Command as a simple string (without quoting of args - so only for display and
	 * logging).
	 */
	string cmd() const;

	const vector<string>& stdout() const;
	const vector<string>& stderr() const;

	int retcode() const;

	/**
	 * Quotes and protects a single string for shell execution.
	 */
	static string quote(const string& str);

    private:

	class Impl;

	std::unique_ptr<Impl> impl;

    };


    inline string quote(const string& str)
    {
	return SystemCmd::quote(str);
    }

}

#endif
