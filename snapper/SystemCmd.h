/*
 * Copyright (c) [2004-2014] Novell, Inc.
 * Copyright (c) [2018-2021] SUSE LLC
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
#include <stdio.h>

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>


namespace snapper
{
    using std::string;
    using std::vector;


    class SystemCmd : private boost::noncopyable
    {
    public:

	SystemCmd(const string& Command_Cv, bool log_output = true);

	virtual ~SystemCmd();

    private:

	enum OutputStream { IDX_STDOUT, IDX_STDERR };

	int execute(const string& Command_Cv);

    public:

	const vector<string>& get_stdout() const { return Lines_aC[IDX_STDOUT]; }
	const vector<string>& get_stderr() const { return Lines_aC[IDX_STDERR]; }

	string cmd() const { return lastCmd; }
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
	int doExecute(const string& Cmd_Cv);
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
	 * Constructs the environment for the child process.
	 *
	 * Must not be called after exec since allocating the memory
	 * for the vector is not allowed then (in a multithreaded
	 * program), see fork(2) and signal-safety(7). So simply call
	 * it right before fork.
	 */
	vector<const char*> make_env() const;

	FILE* File_aC[2];
	vector<string> Lines_aC[2];
	bool NewLineSeen_ab[2];
	bool log_output;
	string lastCmd;
	int Ret_i;
	int Pid_i;
	struct pollfd pfds[2];

	static const unsigned line_limit = 50;
    };


    inline string quote(const string& str)
    {
	return SystemCmd::quote(str);
    }

}

#endif
