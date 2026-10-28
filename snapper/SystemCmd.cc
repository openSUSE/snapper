/*
 * Copyright (c) [2004-2011] Novell, Inc.
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


#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <boost/algorithm/string.hpp>

extern char **environ;

#include "snapper/LoggerImpl.h"
#include "snapper/AppUtil.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Exception.h"


namespace snapper
{
    using namespace std;


    // TODO cleanup implementation, use RAII everywhere, tiny testsuite


    namespace
    {

	/**
	 * Class to temporarily hold copies for execvpe().
	 */
	class TmpForExec
	{
	public:

	    TmpForExec(const vector<char*>& values) : values(values) {}
	    ~TmpForExec();

	    char* const* get() const { return &values[0]; }

	private:

	    vector<char*> values;

	};


	TmpForExec::~TmpForExec()
	{
	    for (char* v : values)
		free(v);
	}

    }


    class SystemCmd::Impl
    {

    public:

	Impl(const Args& args, bool log_output)
	    : args(args), log_output(log_output)
	{
	}

	enum OutputStream { IDX_STDOUT, IDX_STDERR };

	const Args args;
	const bool log_output;

	static const unsigned line_limit = 50;

	string cmd() const;

	bool start();
	bool doWait(int& Ret_ir);
	void wait();
	bool check_status();
	void finish(int status);
	void checkOutput();
	void getUntilEOF(FILE* File_Cr, vector<string>& Lines_Cr, bool& NewLineSeen_br,
			 bool Stderr_bv);
	void extractNewline(const string& Buf_ti, int Cnt_ii, bool& NewLineSeen_br,
			    string& Text_Cr, vector<string>& Lines_Cr);
	void addLine(const string& Text_Cv, vector<string>& Lines_Cr);
	void close_stdin();
	void logOutput() const;
	unsigned numLines(OutputStream Idx_ii) const;
	string getLine(unsigned Num_iv, OutputStream Idx_ii) const;
	bool is_running();
	int retcode() const { return Ret_i; }

	const vector<string>& get_stdout() const { return Lines_aC[IDX_STDOUT]; }
	const vector<string>& get_stderr() const { return Lines_aC[IDX_STDERR]; }

	/**
	 * Constructs the args for the child process.
	 *
	 * Not async-signal-safe, see fork(2) and signal-safety(7).
	 */
	TmpForExec make_args() const;

	/**
	 * Constructs the environment for the child process.
	 *
	 * Not async-signal-safe, see fork(2) and signal-safety(7).
	 */
	TmpForExec make_env() const;

	FILE* File_aC[2] = { NULL, NULL };
	vector<string> Lines_aC[2];
	bool NewLineSeen_ab[2] = { true, true };
	int Ret_i = 0;
	pid_t Pid_i = 0;
	int sin_fd = -1;
	struct pollfd pfds[2];

    };


    SystemCmd::SystemCmd(const Args& args, bool log_output)
	: SystemCmd(args, StartMode::SYNC, log_output)
    {
    }


    SystemCmd::SystemCmd(const Args& args, StartMode mode, bool log_output)
	: impl(make_unique<Impl>(args, log_output))
    {
	y2mil("constructor SystemCmd: " << cmd());

	if (args.get_values().empty())
	    SN_THROW(Exception("args empty"));

	switch (mode)
	{
	    case StartMode::SYNC:
	    {
		Stopwatch stopwatch;

		if (impl->start())
		{
		    impl->wait();
		    y2mil("stopwatch " << stopwatch << " for \"" << cmd() << "\"");
		}
		else
		{
		    impl->Ret_i = -1;
		    y2err("system (\"" << cmd() << "\") = " << impl->Ret_i);
		}
	    }
	    break;

	    case StartMode::ASYNC:
	    {
		impl->start();
	    }
	    break;
	}
    }


    SystemCmd::~SystemCmd()
    {
	impl->close_stdin();

	if (impl->File_aC[Impl::IDX_STDOUT])
	    fclose(impl->File_aC[Impl::IDX_STDOUT]);
	if (impl->File_aC[Impl::IDX_STDERR])
	    fclose(impl->File_aC[Impl::IDX_STDERR]);

	if (impl->Pid_i > 0)
	{
	    int status = 0;
	    if (waitpid(impl->Pid_i, &status, WNOHANG) == 0)
	    {
		kill(impl->Pid_i, SIGTERM);
		waitpid(impl->Pid_i, nullptr, 0);
	    }
	    impl->Pid_i = 0;
	}
    }


    string
    SystemCmd::Impl::cmd() const
    {
	return boost::join(args.get_values(), " ");
    }


    bool
    SystemCmd::Impl::start()
    {
	int sin[2] = { -1, -1 };
	int sout[2] = { -1, -1 };
	int serr[2] = { -1, -1 };

	bool ok_bi = true;

	if (pipe(sin) < 0)
	{
	    y2err("pipe stdin creation failed errno:" << errno << " (" << stringerror(errno) << ")");
	    ok_bi = false;
	}
	if (ok_bi && pipe(sout) < 0)
	{
	    y2err("pipe stdout creation failed errno:" << errno << " (" << stringerror(errno) << ")");
	    ok_bi = false;
	}
	if (ok_bi && pipe(serr) < 0)
	{
	    y2err("pipe stderr creation failed errno:" << errno << " (" << stringerror(errno) << ")");
	    ok_bi = false;
	}

	if (!ok_bi)
	{
	    if (sin[0] >= 0) { close(sin[0]); close(sin[1]); }
	    if (sout[0] >= 0) { close(sout[0]); close(sout[1]); }
	    if (serr[0] >= 0) { close(serr[0]); close(serr[1]); }
	    Ret_i = -1;
	    return false;
	}

	pfds[0].fd = sout[0];
	pfds[0].events = POLLIN;
	if (fcntl(pfds[0].fd, F_SETFL, O_NONBLOCK) < 0)
	{
	    y2err("fcntl O_NONBLOCK failed errno:" << errno << " (" << stringerror(errno) << ")");
	}
	pfds[1].fd = serr[0];
	pfds[1].events = POLLIN;
	if (fcntl(pfds[1].fd, F_SETFL, O_NONBLOCK) < 0)
	{
	    y2err("fcntl O_NONBLOCK failed errno:" << errno << " (" << stringerror(errno) << ")");
	}
	y2deb("sout:" << pfds[0].fd << " serr:" << pfds[1].fd);

	const int max_fd = getdtablesize();

	const TmpForExec args_p(make_args());
	const TmpForExec env_p(make_env());

	switch ((Pid_i = fork()))
	{
	    case 0:
	    {
		// Do not use exit() here. Use _exit() instead.

		// Only use async-signal-safe functions here, see fork(2) and
		// signal-safety(7).

		if (dup2(sin[0], STDIN_FILENO) < 0)
		    _exit(125);
		if (dup2(sout[1], STDOUT_FILENO) < 0)
		    _exit(125);
		if (dup2(serr[1], STDERR_FILENO) < 0)
		    _exit(125);

		if (close(sin[0]) < 0)
		    _exit(125);
		if (close(sin[1]) < 0)
		    _exit(125);
		if (close(sout[0]) < 0)
		    _exit(125);
		if (close(sout[1]) < 0)
		    _exit(125);
		if (close(serr[0]) < 0)
		    _exit(125);
		if (close(serr[1]) < 0)
		    _exit(125);

		for (int fd = 3; fd < max_fd; ++fd)
		    close(fd);

		execvpe(args.get_values()[0].c_str(), args_p.get(), env_p.get());

		if (errno == ENOENT)
		    _exit(127);
		if (errno == ENOEXEC || errno == EACCES || errno == EISDIR)
		    _exit(126);
		_exit(125);
	    }
	    break;

	    case -1:
	    {
		close(sin[0]);
		close(sin[1]);
		close(sout[0]);
		close(sout[1]);
		close(serr[0]);
		close(serr[1]);
		Ret_i = -1;
		y2err("fork failed errno:" << errno << " (" << stringerror(errno) << ")");
		return false;
	    }
	    break;

	    default:
	    {
		close(sin[0]);
		sin_fd = sin[1];

		if (close(sout[1]) < 0)
		    y2err("close parent failed errno:" << errno << " (" << stringerror(errno) << ")");
		if (close(serr[1]) < 0)
		    y2err("close parent failed errno:" << errno << " (" << stringerror(errno) << ")");

		Ret_i = 0;
		File_aC[IDX_STDOUT] = fdopen(sout[0], "r");
		if (File_aC[IDX_STDOUT] == NULL)
		    y2err("fdopen stdout failed errno:" << errno << " (" << stringerror(errno) << ")");

		File_aC[IDX_STDERR] = fdopen(serr[0], "r");
		if (File_aC[IDX_STDERR] == NULL)
		    y2err("fdopen stderr failed errno:" << errno << " (" << stringerror(errno) << ")");

		return true;
	    }
	    break;
	}

	return false;
    }


    void
    SystemCmd::Impl::finish(int status)
    {
	checkOutput();

	if (File_aC[IDX_STDOUT])
	{
	    fclose(File_aC[IDX_STDOUT]);
	    File_aC[IDX_STDOUT] = NULL;
	    pfds[0].fd = -1;
	}
	if (File_aC[IDX_STDERR])
	{
	    fclose(File_aC[IDX_STDERR]);
	    File_aC[IDX_STDERR] = NULL;
	    pfds[1].fd = -1;
	}

	close_stdin();

	if (WIFEXITED(status))
	{
	    Ret_i = WEXITSTATUS(status);
	    if (Ret_i == 126)
		y2err("command \"" << cmd() << "\" not executable");
	    else if (Ret_i == 127)
		y2err("command \"" << cmd() << "\" not found");
	}
	else
	{
	    Ret_i = -127;
	    y2err("command \"" << cmd() << "\" failed");
	}

	y2mil("system() Returns:" << Ret_i);
	if (Ret_i != 0 && log_output)
	    logOutput();

	Pid_i = 0;
    }


    bool
    SystemCmd::Impl::check_status()
    {
	if (Pid_i <= 0)
	    return false;

	int status = 0;
	int wait_ret = waitpid(Pid_i, &status, WNOHANG);
	if (wait_ret == 0)
	{
	    return true;
	}
	else if (wait_ret > 0)
	{
	    finish(status);
	    return false;
	}
	else
	{
	    if (errno == ECHILD)
	    {
		finish(0);
	    }
	    else
	    {
		y2err("waitpid failed errno:" << errno << " (" << stringerror(errno) << ")");
	    }
	    return false;
	}
    }


    bool
    SystemCmd::Impl::doWait(int& Ret_ir)
    {
	if (Pid_i <= 0)
	{
	    Ret_ir = Ret_i;
	    return true;
	}

	int Wait_ii = 0;
	int Status_ii = 0;

	do
	{
	    y2deb("[0] fd:" << pfds[0].fd << " ev:" << hex << (unsigned)(pfds[0].events) << dec << " "
		  "[1] fd:" << pfds[1].fd << " ev:" << hex << (unsigned)(pfds[1].events));
	    int sel = poll(pfds, 2, 1000);
	    if (sel < 0)
	    {
		if (errno != EINTR)
		    y2err("poll failed errno:" << errno << " (" << stringerror(errno) << ")");
	    }
	    y2deb("poll ret:" << sel);
	    if (sel > 0)
	    {
		checkOutput();
	    }
	    Wait_ii = waitpid(Pid_i, &Status_ii, WNOHANG);
	    y2deb("Wait ret:" << Wait_ii);
	}
	while (Wait_ii == 0);

	if (Wait_ii > 0)
	{
	    finish(Status_ii);
	    Ret_ir = Ret_i;
	}
	else
	{
	    Ret_ir = -127;
	    Ret_i = -127;
	    Pid_i = 0;
	    y2err("command \"" << cmd() << "\" failed");
	}

	y2deb("Wait:" << Wait_ii << " stat:" << Status_ii << " Ret:" << Ret_ir);
	return Wait_ii > 0;
    }


    void
    SystemCmd::Impl::wait()
    {
	if (Pid_i <= 0)
	    return;

	close_stdin();
	doWait(Ret_i);
    }


    bool
    SystemCmd::Impl::is_running()
    {
	if (Pid_i <= 0)
	    return false;

	check_status();
	return Pid_i > 0;
    }


    void
    SystemCmd::Impl::close_stdin()
    {
	if (sin_fd >= 0)
	{
	    close(sin_fd);
	    sin_fd = -1;
	}
    }


    void
    SystemCmd::Impl::checkOutput()
    {
	y2deb("NewLine out:" << NewLineSeen_ab[IDX_STDOUT] << " err:" << NewLineSeen_ab[IDX_STDERR]);
	if (File_aC[IDX_STDOUT])
	    getUntilEOF(File_aC[IDX_STDOUT], Lines_aC[IDX_STDOUT], NewLineSeen_ab[IDX_STDOUT], false);
	if (File_aC[IDX_STDERR])
	    getUntilEOF(File_aC[IDX_STDERR], Lines_aC[IDX_STDERR], NewLineSeen_ab[IDX_STDERR], true);
	y2deb("NewLine out:" << NewLineSeen_ab[IDX_STDOUT] << " err:" << NewLineSeen_ab[IDX_STDERR]);
    }


    unsigned
    SystemCmd::Impl::numLines(OutputStream Idx_iv) const
    {
	unsigned Ret_ii;

	if (Idx_iv > 1)
	{
	    y2err("invalid index " << Idx_iv);
	}
	Ret_ii = Lines_aC[Idx_iv].size();
	y2deb("ret:" << Ret_ii);
	return Ret_ii;
    }


    string
    SystemCmd::Impl::getLine(unsigned Nr_iv, OutputStream Idx_iv) const
    {
	string ret;

	if (Idx_iv > 1)
	{
	    y2err("invalid index " << Idx_iv);
	}
	if (Nr_iv < Lines_aC[Idx_iv].size())
	{
	    ret = Lines_aC[Idx_iv][Nr_iv];
	}

	return ret;
    }


#define BUF_LEN 256

    void
    SystemCmd::Impl::getUntilEOF(FILE* File_Cr, vector<string>& Lines_Cr, bool& NewLine_br,
				 bool Stderr_bv)
    {
	size_t old_size = Lines_Cr.size();
	char Buf_ti[BUF_LEN];
	int Cnt_ii;
	int Char_ii;
	string Text_Ci;

	clearerr(File_Cr);
	Cnt_ii = 0;
	Char_ii = EOF;
	while ((Char_ii = fgetc(File_Cr)) != EOF)
	{
	    Buf_ti[Cnt_ii++] = Char_ii;
	    if (Cnt_ii == sizeof(Buf_ti) - 1)
	    {
		Buf_ti[Cnt_ii] = 0;
		extractNewline(Buf_ti, Cnt_ii, NewLine_br, Text_Ci, Lines_Cr);
		Cnt_ii = 0;
	    }
	    Char_ii = EOF;
	}
	if (Cnt_ii > 0)
	{
	    Buf_ti[Cnt_ii] = 0;
	    extractNewline(Buf_ti, Cnt_ii, NewLine_br, Text_Ci, Lines_Cr);
	}
	if (Text_Ci.length() > 0)
	{
	    if (NewLine_br)
	    {
		addLine(Text_Ci, Lines_Cr);
	    }
	    else
	    {
		Lines_Cr[Lines_Cr.size() - 1] += Text_Ci;
	    }
	    NewLine_br = false;
	}
	else
	{
	    NewLine_br = true;
	}
	y2deb("Text_Ci:" << Text_Ci << " NewLine:" << NewLine_br);
	if (old_size != Lines_Cr.size())
	{
	    y2mil("pid:" << Pid_i << " added lines:" << Lines_Cr.size() - old_size << " stderr:" << Stderr_bv);
	}
    }


    void
    SystemCmd::Impl::extractNewline(const string& Buf_ti, int Cnt_iv, bool& NewLine_br,
				    string& Text_Cr, vector<string>& Lines_Cr)
    {
	string::size_type Idx_ii;

	Text_Cr += Buf_ti;
	while ((Idx_ii = Text_Cr.find('\n')) != string::npos)
	{
	    if (!NewLine_br)
	    {
		Lines_Cr[Lines_Cr.size() - 1] += Text_Cr.substr(0, Idx_ii);
	    }
	    else
	    {
		addLine(Text_Cr.substr(0, Idx_ii), Lines_Cr);
	    }
	    Text_Cr.erase(0, Idx_ii + 1);
	    NewLine_br = true;
	}
	y2deb("Text_Ci:" << Text_Cr << " NewLine:" << NewLine_br);
    }


    void
    SystemCmd::Impl::addLine(const string& Text_Cv, vector<string>& Lines_Cr)
    {
	if (log_output)
	{
	    if (Lines_Cr.size() < line_limit)
	    {
		y2mil("Adding Line " << Lines_Cr.size() + 1 << " \"" << Text_Cv << "\"");
	    }
	    else
	    {
		y2deb("Adding Line " << Lines_Cr.size() + 1 << " \"" << Text_Cv << "\"");
	    }
	}

	Lines_Cr.push_back(Text_Cv);
    }


    void
    SystemCmd::Impl::logOutput() const
    {
	unsigned lines = numLines(IDX_STDERR);
	if (lines <= line_limit)
	{
	    for (unsigned i = 0; i < lines; ++i)
		y2mil("stderr:" << getLine(i, IDX_STDERR));
	}
	else
	{
	    for (unsigned i = 0; i < line_limit / 2; ++i)
		y2mil("stderr:" << getLine(i, IDX_STDERR));
	    y2mil("stderr omitting lines");
	    for (unsigned i = lines - line_limit / 2; i < lines; ++i)
		y2mil("stderr:" << getLine(i, IDX_STDERR));
	}

	lines = numLines(IDX_STDOUT);
	if (lines <= line_limit)
	{
	    for (unsigned i = 0; i < lines; ++i)
		y2mil("stdout:" << getLine(i, IDX_STDOUT));
	}
	else
	{
	    for (unsigned i = 0; i < line_limit / 2; ++i)
		y2mil("stdout:" << getLine(i, IDX_STDOUT));
	    y2mil("stdout omitting lines");
	    for (unsigned i = lines - line_limit / 2; i < lines; ++i)
		y2mil("stdout:" << getLine(i, IDX_STDOUT));
	}
    }


    TmpForExec
    SystemCmd::Impl::make_args() const
    {
	vector<char*> ret;

	for (const string& v : args.get_values())
	{
	    ret.push_back(strdup(v.c_str()));
	}

	ret.push_back(nullptr);

	return ret;
    }


    TmpForExec
    SystemCmd::Impl::make_env() const
    {
	vector<char*> ret;

	for (char** v = environ; *v != NULL; ++v)
	{
	    if (strncmp(*v, "LC_ALL=", strlen("LC_ALL=")) != 0 &&
		strncmp(*v, "LANGUAGE=", strlen("LANGUAGE=")) != 0)
		ret.push_back(strdup(*v));
	}

	ret.push_back(strdup("LC_ALL=C"));
	ret.push_back(strdup("LANGUAGE=C"));

	ret.push_back(nullptr);

	return ret;
    }


    string
    SystemCmd::cmd() const
    {
	return impl->cmd();
    }


    const vector<string>&
    SystemCmd::get_stdout() const
    {
	return impl->get_stdout();
    }


    const vector<string>&
    SystemCmd::get_stderr() const
    {
	return impl->get_stderr();
    }


    int
    SystemCmd::retcode() const
    {
	return impl->retcode();
    }


    string
    SystemCmd::quote(const string& str)
    {
	return "'" + boost::replace_all_copy(str, "'", "'\\''") + "'";
    }

}
