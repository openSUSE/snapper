/*
 * Copyright (c) [2004-2014] Novell, Inc.
 * Copyright (c) [2018-2023] SUSE LLC
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

#include <poll.h>
#include <cstdio>
#include <string>
#include <vector>
#include <initializer_list>
#include <boost/noncopyable.hpp>


namespace snapper
{
    using std::string;
    using std::vector;


    class SystemCmd final : private boost::noncopyable
    {
    public:


	/**
	 * Class holding command with arguments.
	 */
	class Args
	{
	public:

	    Args(std::initializer_list<string> init)
		: values(init) {}

	    const vector<string>& get_values() const { return values; }

	    Args& operator<<(const char* arg) { values.push_back(arg); return *this; }
	    Args& operator<<(const string& arg) { values.push_back(arg); return *this; }

	    Args& operator<<(const vector<string>& args)
		{ values.insert(values.end(), args.begin(), args.end()); return *this; }

	private:

	    vector<string> values;

	};


	SystemCmd(const Args& args, bool log_output = true);

	~SystemCmd();

    private:

	enum OutputStream { IDX_STDOUT, IDX_STDERR };

    public:

	const vector<string>& get_stdout() const { return Lines_aC[IDX_STDOUT]; }
	const vector<string>& get_stderr() const { return Lines_aC[IDX_STDERR]; }

	/**
	 * Command as a simple string (without quoting of args - so only for display and
	 * logging).
	 */
	string cmd() const;

	int retcode() const { return Ret_i; }

    private:

	unsigned numLines(OutputStream Idx_ii = IDX_STDOUT) const;
	string getLine(unsigned Num_iv, OutputStream Idx_ii = IDX_STDOUT) const;

    public:

	/**
	 * Quotes and protects a single string for shell execution.
	 */
	static string quote(const string& str);

    private:

	void invalidate();
	void closeOpenFds() const;
	void execute();
	bool doWait(int& Ret_ir);
	void checkOutput();
	void getUntilEOF(FILE* File_Cr, vector<string>& Lines_Cr, bool& NewLineSeen_br,
			 bool Stderr_bv);
	void extractNewline(const string& Buf_ti, int Cnt_ii, bool& NewLineSeen_br,
			    string& Text_Cr, vector<string>& Lines_Cr);
	void addLine(const string& Text_Cv, vector<string>& Lines_Cr);
	void init();

	void logOutput() const;


	/**
	 * Class to tempararily hold copies for execle() and execvpe().
	 */
	class TmpForExec
	{
	public:

	    TmpForExec(const vector<char*>& values) : values(values) {}
	    ~TmpForExec();

	    char* const * get() const { return &values[0]; }

	private:

	    vector<char*> values;

	};


	/**
	 * Constructs the args for the child process.
	 *
	 * Must not be called after exec since allocating the memory
	 * for the vector is not allowed then (in a multithreaded
	 * program), see fork(2) and signal-safety(7). So simply call
	 * it right before fork.
	 */
	TmpForExec make_args() const;

	/**
	 * Constructs the environment for the child process.
	 *
	 * Same not as for make_args().
	 */
	TmpForExec make_env() const;


	const Args args;
	const bool log_output;

	FILE* File_aC[2];
	vector<string> Lines_aC[2];
	bool NewLineSeen_ab[2];
	int Ret_i = 0;
	int Pid_i = 0;
	struct pollfd pfds[2];

	static const unsigned line_limit = 50;

    };


    inline string quote(const string& str)
    {
	return SystemCmd::quote(str);
    }

}

#endif
