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


#define SYSCALL_FAILED(SYSCALL_MSG) \
    SN_THROW(Exception(Exception::strErrno(errno, SYSCALL_MSG)))


// See man bash
// Since all commands are started via a shell, only the shell's return value is returned.
#define SHELL_RET_COMMAND_NOT_EXECUTABLE	126
#define SHELL_RET_COMMAND_NOT_FOUND		127
#define SHELL_RET_SIGNAL			128


namespace snapper
{
    using namespace std;


    namespace
    {

	class FileDescriptor : boost::noncopyable
	{
	public:

	    ~FileDescriptor() { if (fd >= 0) close(); }

	    int close();

	    int fd = -1;

	};


	int
	FileDescriptor::close()
	{
	    int ret = ::close(fd);
	    fd = -1;
	    return ret;
	}


	class Pipe : boost::noncopyable
	{
	public:

	    Pipe(int flags = O_CLOEXEC);

	    FileDescriptor read_end;
	    FileDescriptor write_end;

	};


	Pipe::Pipe(int flags)
	{
	    int pipefd[2];

	    if (pipe2(pipefd, flags) != 0)
		SYSCALL_FAILED("pipe2 failed");

	    read_end.fd = pipefd[0];
	    write_end.fd = pipefd[1];
	}


	/**
	 * Struct to hold information about failures of the child between fork and exec. The
	 * struct is passed via a pipe from the client to the parent. The information is so
	 * far only used for logging.
	 */
	struct ChildFailureInfo
	{
	    int cmdnum = 0;
	    int errnum = 0;
	};


	/**
	 * RAII for pid of child.
	 */
	class Child : boost::noncopyable
	{
	public:

	    ~Child()
	    {
		if (pid != -1)
		    waitpid(NULL);
	    }

	    int get_pid() const { return pid; }

	    int fork();
	    int waitpid(int* wstatus);

	private:

	    int pid = -1;

	};


	int
	Child::fork()
	{
	    pid = ::fork();
	    return pid;
	}


	int
	Child::waitpid(int* wstatus)
	{
	    int ret = TEMP_FAILURE_RETRY(::waitpid(pid, wstatus, 0));

	    // Since waitpid is called with options = 0 the child is dead if waitpid
	    // does not report an error.

	    if (ret != -1)
		pid = -1;

	    return ret;
	}

    }


    class SystemCmd::Impl
    {
    public:

	Impl(const Args& args, bool log_output);

	string cmd() const;

	const vector<string>& stdout() const { return stdout_lines; }
	const vector<string>& stderr() const { return stderr_lines; }

	int retcode() const { return child_retcode; }

	void step_fork_and_exec();
	void step_poll();
	void step_wait();

    private:

	void read_stdout();
	void read_stderr();

	void fill_buffer(const char* name, int fd, string& buffer, vector<string>& lines) const;
	void split_buffer(const char* name, string& buffer, vector<string>& lines, bool finito = false) const;
	void add_line(const char* name, const string& line, vector<string>& lines) const;

	void log_output_summary() const;

	/**
	 * Constructs the args for the child process.
	 *
	 * Not async-signal-safe, see fork(2) and signal-safety(7).
	 */
	vector<const char*> make_args() const;

	/**
	 * Constructs the environment for the child process.
	 *
	 * Not async-signal-safe, see fork(2) and signal-safety(7).
	 */
	vector<const char*> make_env() const;

	const Args args;
	const bool log_output;

	static constexpr unsigned int line_limit = 50;

	// Child must be places before Pipe! Otherwise in async mode stdin is not closed
	// before waitpid and the destructor hangs.

	vector<string> envs;

	Child child;

	Pipe stdin_pipe;
	Pipe stdout_pipe;
	Pipe stderr_pipe;

	struct pollfd pollfds[2];

	string stdout_buffer;
	string stderr_buffer;

	vector<string> stdout_lines;
	vector<string> stderr_lines;

	int child_retcode = 0;

    };


    SystemCmd::Impl::Impl(const Args& args, bool log_output)
	: args(args), log_output(log_output)
    {
	for (char** v = environ; *v != nullptr; ++v)
	{
	    if (strncmp(*v, "LC_ALL=", strlen("LC_ALL=")) != 0 &&
		strncmp(*v, "LANGUAGE=", strlen("LANGUAGE=")) != 0)
		envs.push_back(*v);
	}

	envs.push_back("LC_ALL=C");
	envs.push_back("LANGUAGE=C");
    }


    string
    SystemCmd::Impl::cmd() const
    {
	return boost::join(args.get_values(), " ");
    }


    void
    SystemCmd::Impl::step_fork_and_exec()
    {
	y2deb("step fork and exec");

	if (fcntl(stdin_pipe.write_end.fd, F_SETFL, O_NONBLOCK) != 0)
	    SYSCALL_FAILED("fcntl stdin O_NONBLOCK failed");

	if (fcntl(stdout_pipe.read_end.fd, F_SETFL, O_NONBLOCK) != 0)
	    SYSCALL_FAILED("fcntl stdout O_NONBLOCK failed");

	if (fcntl(stderr_pipe.read_end.fd, F_SETFL, O_NONBLOCK) != 0)
	    SYSCALL_FAILED("fcntl stderr O_NONBLOCK failed");

	Pipe child_failure_info_pipe;

	const int max_fd = getdtablesize();

	const vector<const char*> args_p(make_args());
	const vector<const char*> env_p(make_env());

	child.fork();
	if (child.get_pid() == -1)
	    SYSCALL_FAILED("fork failed");

	if (child.get_pid() == 0)
	{
	    // child process

	    // Do not use exit() here. Use _exit() instead. Thus no destructors are
	    // called.

	    // Only use async-signal-safe functions here, see fork(2) and
	    // signal-safety(7).

	    if (dup2(stdin_pipe.read_end.fd, STDIN_FILENO) < 0)
		_exit(125);
	    if (stdin_pipe.write_end.close() != 0)
		_exit(125);

	    if (dup2(stdout_pipe.write_end.fd, STDOUT_FILENO) < 0)
		_exit(125);
	    if (stdout_pipe.read_end.close() != 0)
		_exit(125);

	    if (dup2(stderr_pipe.write_end.fd, STDERR_FILENO) < 0)
		_exit(125);
	    if (stderr_pipe.read_end.close() != 0)
		_exit(125);

	    if (child_failure_info_pipe.read_end.close() != 0)
		_exit(125);

	    // Unfortunately close_range(2) is still too new. It is also not mentioned in
	    // signal-safety(7). We use CLOEXEC here since child_failure_info_pipe must
	    // not yet be closed.

	    for (int fd = 3; fd < max_fd; ++fd)
		fcntl(fd, F_SETFD, FD_CLOEXEC);

	    // The const_casts below should be fine since the exec functions do not modify
	    // "pointers and the strings to which those arrays point". Assume that also
	    // applies to GNU execvpe.
	    // https://pubs.opengroup.org/onlinepubs/9699919799/functions/exec.html

	    execvpe(args.get_values()[0].c_str(), const_cast<char* const*>(args_p.data()),
		    const_cast<char* const*>(env_p.data()));

	    // If we get here the exec has failed. Depending on errno we return an
	    // error code like the shell does ('sh -c ...').

	    ChildFailureInfo child_failure_info { 1, errno };
	    int write_ret = TEMP_FAILURE_RETRY(write(child_failure_info_pipe.write_end.fd,
						     &child_failure_info, sizeof(child_failure_info)));
	    if (write_ret != sizeof(child_failure_info))
		_exit(125);
	    if (child_failure_info_pipe.write_end.close() != 0)
		_exit(125);

	    if (errno == ENOENT)
		_exit(SHELL_RET_COMMAND_NOT_FOUND);
	    if (errno == ENOEXEC || errno == EACCES || errno == EISDIR)
		_exit(SHELL_RET_COMMAND_NOT_EXECUTABLE);
	    _exit(125);
	}

	// parent process

	y2mil("child.pid:" << child.get_pid());

	if (stdin_pipe.read_end.close() != 0)
	    SYSCALL_FAILED("close stdin in parent failed");

	if (stdout_pipe.write_end.close() != 0)
	    SYSCALL_FAILED("close stdout in parent failed");

	if (stderr_pipe.write_end.close() != 0)
	    SYSCALL_FAILED("close stderr in parent failed");

	if (child_failure_info_pipe.write_end.close() != 0)
	    SYSCALL_FAILED("close child_failure_info_pipe failed");

	// Read ChildFailureInfo. If the exec in the child succeeded the pipe
	// was closed (due to CLOEXEC) and reading fails. If the exec in the
	// child failed the information was written to the pipe and can be
	// used here in the parent.

	ChildFailureInfo child_failure_info;
	int read_ret = TEMP_FAILURE_RETRY(read(child_failure_info_pipe.read_end.fd, &child_failure_info,
					       sizeof(child_failure_info)));
	if (read_ret < 0)
	    SYSCALL_FAILED("read child_failure_info");

	if (read_ret == sizeof(child_failure_info))
	{
	    y2err("exec failed: " << child_failure_info.errnum);
	}

	if (child_failure_info_pipe.read_end.close() != 0)
	    SYSCALL_FAILED("close child_failure_info_pipe failed");
    }


    void
    SystemCmd::Impl::step_poll()
    {
	stdin_pipe.write_end.close();

	pollfds[0].events = POLLIN;
	pollfds[0].fd = stdout_pipe.read_end.fd;

	pollfds[1].events = POLLIN;
	pollfds[1].fd = stderr_pipe.read_end.fd;

	while (pollfds[0].fd != -1 || pollfds[1].fd != -1)
	{
	    y2deb("pollfds[0].fd:" << pollfds[0].fd << " pollfds[1].fd:" << pollfds[1].fd);

	    int poll_ret = TEMP_FAILURE_RETRY(poll(pollfds, 2, -1));
	    if (poll_ret < 0)
		SYSCALL_FAILED("poll failed");

	    if (poll_ret > 0)
	    {
		y2deb("pollfds[0].revents:" << pollfds[0].revents << " pollfds[1].revents:" << pollfds[1].revents);

		if (pollfds[0].revents & POLLIN)
		    read_stdout();
		if (pollfds[0].revents & POLLHUP)
		    pollfds[0].fd = -1;

		if (pollfds[1].revents & POLLIN)
		    read_stderr();
		if (pollfds[1].revents & POLLHUP)
		    pollfds[1].fd = -1;
	    }
	}

	stdout_pipe.read_end.close();
	split_buffer("stdout", stdout_buffer, stdout_lines, true);
	if (stdout_lines.size() >= line_limit)
	    y2mil("stdout lines:" << stdout_lines.size());

	stderr_pipe.read_end.close();
	split_buffer("stderr", stderr_buffer, stderr_lines, true);
	if (stderr_lines.size() >= line_limit)
	    y2mil("stderr lines:" << stderr_lines.size());
    }


    void
    SystemCmd::Impl::step_wait()
    {
	y2deb("step wait");

	int wstatus = 0;
	int waitpid_ret = child.waitpid(&wstatus);
	if (waitpid_ret < 0)
	    SYSCALL_FAILED("waitpid failed");

	y2deb("waitpid_ret:" << waitpid_ret << " wstatus:" << wstatus);

	if (WIFEXITED(wstatus))
	{
	    child_retcode = WEXITSTATUS(wstatus);

	    if (child_retcode == SHELL_RET_COMMAND_NOT_EXECUTABLE)
		y2err("command \"" << cmd() << "\" not executable");
	    else if (child_retcode == SHELL_RET_COMMAND_NOT_FOUND)
		y2err("command \"" << cmd() << "\" not found");
	    else if (child_retcode > SHELL_RET_SIGNAL)
		y2err("command \"" << cmd() << "\" caught signal");
	}
	else if (WIFSIGNALED(wstatus))
	{
	    child_retcode = -127;
	    y2err("command \"" << cmd() << "\" caught signal");
	}
	else
	{
	    child_retcode = -127;
	    y2err("command \"" << cmd() << "\" failed");
	}

	y2mil("child_retcode:" << child_retcode);

	if (child_retcode != 0 && log_output)
	    log_output_summary();
    }


    void
    SystemCmd::Impl::read_stdout()
    {
	fill_buffer("stdout", stdout_pipe.read_end.fd, stdout_buffer, stdout_lines);
    }


    void
    SystemCmd::Impl::read_stderr()
    {
	fill_buffer("stderr", stderr_pipe.read_end.fd, stderr_buffer, stderr_lines);
    }


    void
    SystemCmd::Impl::fill_buffer(const char* name, int fd, string& buffer, vector<string>& lines) const
    {
	// We need a loop here to read all data from fd since poll can set POLLIN and
	// POLLHUP at the same time. Otherwise we can loose data.

	char buffer2[1024];

	while (true)
	{
	    ssize_t read_ret = TEMP_FAILURE_RETRY(read(fd, buffer2, sizeof(buffer2)));
	    if (read_ret < 0)
	    {
		if (errno == EAGAIN)
		    break;

		SYSCALL_FAILED("read failed");
	    }

	    buffer.append(buffer2, read_ret);

	    split_buffer(name, buffer, lines);

	    if ((size_t)(read_ret) < sizeof(buffer2))
		break;
	}
    }


    void
    SystemCmd::Impl::split_buffer(const char* name, string& buffer, vector<string>& lines, bool finito) const
    {
	while (true)
	{
	    string::size_type pos = buffer.find('\n');
	    if (pos == string::npos)
		break;

	    add_line(name, buffer.substr(0, pos), lines);
	    buffer.erase(0, pos + 1);
	}

	if (finito && !buffer.empty())
	    add_line(name, buffer, lines);
    }


    void
    SystemCmd::Impl::add_line(const char* name, const string& line, vector<string>& lines) const
    {
	if (log_output)
	{
	    if (lines.size() < line_limit)
		y2mil("line " << name << "[" << lines.size() << "] '" << line << "'");
	    else
		y2deb("line " << name << "[" << lines.size() << "] '" << line << "'");
	}

	lines.push_back(line);
    }


    void
    SystemCmd::Impl::log_output_summary() const
    {
	if (stderr_lines.size() <= line_limit)
	{
	    for (const string& line : stderr_lines)
		y2mil("stderr:" << line);
	}
	else
	{
	    for (size_t i = 0; i < line_limit / 2; ++i)
		y2mil("stderr:" << stderr_lines[i]);
	    y2mil("stderr omitting lines");
	    for (size_t i = stderr_lines.size() - line_limit / 2; i < stderr_lines.size(); ++i)
		y2mil("stderr:" << stderr_lines[i]);
	}

	if (stdout_lines.size() <= line_limit)
	{
	    for (const string& line : stdout_lines)
		y2mil("stdout:" << line);
	}
	else
	{
	    for (size_t i = 0; i < line_limit / 2; ++i)
		y2mil("stdout:" << stdout_lines[i]);
	    y2mil("stdout omitting lines");
	    for (size_t i = stdout_lines.size() - line_limit / 2; i < stdout_lines.size(); ++i)
		y2mil("stdout:" << stdout_lines[i]);
	}
    }


    vector<const char*>
    SystemCmd::Impl::make_args() const
    {
	vector<const char*> args_p;
	args_p.reserve(args.get_values().size() + 1);

	for (const string& v : args.get_values())
	    args_p.push_back(v.c_str());
	args_p.push_back(nullptr);

	return args_p;
    }


    vector<const char*>
    SystemCmd::Impl::make_env() const
    {
	vector<const char*> envs_p;
	envs_p.reserve(envs.size() + 1);

	for (const string& v : envs)
	    envs_p.push_back(v.c_str());
	envs_p.push_back(nullptr);

	return envs_p;
    }


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

		impl->step_fork_and_exec();
		impl->step_poll();
		impl->step_wait();

		y2mil("stopwatch " << stopwatch << " for \"" << cmd() << "\"");
	    }
	    break;

	    case StartMode::ASYNC:
	    {
		impl->step_fork_and_exec();
	    }
	    break;
	}
    }


    SystemCmd::~SystemCmd() = default;


    string
    SystemCmd::cmd() const
    {
	return impl->cmd();
    }


    const vector<string>&
    SystemCmd::stdout() const
    {
	return impl->stdout();
    }


    const vector<string>&
    SystemCmd::stderr() const
    {
	return impl->stderr();
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
