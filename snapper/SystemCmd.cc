/*
 * Copyright (c) [2004-2011] Novell, Inc.
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


#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string>
#include <boost/algorithm/string.hpp>

extern char **environ;

#include "snapper/Log.h"
#include "snapper/AppUtil.h"
#include "snapper/SystemCmd.h"
#include "snapper/SnapperDefines.h"


namespace snapper
{
    using namespace std;


    SystemCmd::SystemCmd(const string& Command_Cv, bool log_output)
	: log_output(log_output)
{
    y2mil("constructor SystemCmd:\"" << Command_Cv << "\"");
    init();
    execute( Command_Cv );
}


void SystemCmd::init()
    {
    File_aC[0] = File_aC[1] = NULL;
    pfds[0].events = pfds[1].events = POLLIN;
    }


SystemCmd::~SystemCmd()
    {
    if( File_aC[IDX_STDOUT] )
	fclose( File_aC[IDX_STDOUT] );
    if( File_aC[IDX_STDERR] )
	fclose( File_aC[IDX_STDERR] );
    }


void
SystemCmd::closeOpenFds() const
    {
    int max_fd = getdtablesize();
    for( int fd = 3; fd < max_fd; fd++ )
	{
	close(fd);
	}
    }


int
SystemCmd::execute(const string& Cmd_Cv)
{
    y2mil("SystemCmd Executing:\"" << Cmd_Cv << "\"");
    return doExecute(Cmd_Cv);
}


int
SystemCmd::doExecute( const string& Cmd )
    {
    lastCmd = Cmd;
    y2deb("Cmd:" << Cmd);

    StopWatch stopwatch;

    File_aC[IDX_STDERR] = File_aC[IDX_STDOUT] = NULL;
    invalidate();
    int sout[2];
    int serr[2];
    bool ok_bi = true;
    if( pipe(sout)<0 )
	{
	y2err("pipe stdout creation failed errno:" << errno << " (" << stringerror(errno) << ")");
	ok_bi = false;
	}
    if( pipe(serr)<0 )
	{
	y2err("pipe stderr creation failed errno:" << errno << " (" << stringerror(errno) << ")");
	ok_bi = false;
	}
    if( ok_bi )
	{
	pfds[0].fd = sout[0];
	if( fcntl( pfds[0].fd, F_SETFL, O_NONBLOCK )<0 )
	    {
	    y2err("fcntl O_NONBLOCK failed errno:" << errno << " (" << stringerror(errno) << ")");
	    }
	pfds[1].fd = serr[0];
	if( fcntl( pfds[1].fd, F_SETFL, O_NONBLOCK )<0 )
	    {
	    y2err("fcntl O_NONBLOCK failed errno:" << errno << " (" << stringerror(errno) << ")");
	    }
	y2deb("sout:" << pfds[0].fd << " serr:" << pfds[1].fd);

	const vector<const char*> env = make_env();

	switch( (Pid_i=fork()) )
	    {
	    case 0:
		if( dup2( sout[1], STDOUT_FILENO )<0 )
		    {
		    y2err("dup2 stdout child failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		if( dup2( serr[1], STDERR_FILENO )<0 )
		    {
		    y2err("dup2 stderr child failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		if( close( sout[0] )<0 )
		    {
		    y2err("close child failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		if( close( serr[0] )<0 )
		    {
		    y2err("close child failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		closeOpenFds();
		Ret_i = execle(SH_BIN, SH_BIN, "-c", Cmd.c_str(), nullptr, &env[0]);
		y2err("SHOULD NOT HAPPEN \"" SH_BIN "\" Ret:" << Ret_i);
		break;
	    case -1:
		Ret_i = -1;
		break;
	    default:
		if( close( sout[1] )<0 )
		    {
		    y2err("close parent failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		if( close( serr[1] )<0 )
		    {
		    y2err("close parent failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		Ret_i = 0;
		File_aC[IDX_STDOUT] = fdopen( sout[0], "r" );
		if( File_aC[IDX_STDOUT] == NULL )
		    {
		    y2err("fdopen stdout failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		File_aC[IDX_STDERR] = fdopen( serr[0], "r" );
		if( File_aC[IDX_STDERR] == NULL )
		    {
			y2err("fdopen stderr failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }

		doWait( Ret_i );
		y2mil("stopwatch " << stopwatch << " for \"" << cmd() << "\"");

		break;
	    }
	}
    else
	{
	Ret_i = -1;
	}
    if( Ret_i==-127 || Ret_i==-1 )
	{
	y2err("system (\"" << Cmd << "\") = " << Ret_i);
	}
    checkOutput();
    y2mil("system() Returns:" << Ret_i);
    if (Ret_i != 0 && log_output)
	logOutput();
    return Ret_i;
    }


bool
SystemCmd::doWait( int& Ret_ir )
    {
    int Wait_ii;
    int Status_ii;

    do
	{
	y2deb("[0] fd:" << pfds[0].fd << " ev:" << hex << (unsigned)(pfds[0].events) << dec << " "
	      "[1] fd:" << pfds[1].fd << " ev:" << hex << (unsigned)(pfds[1].events));
	int sel = poll( pfds, 2, 1000 );
	if (sel < 0)
	    {
	    y2err("poll failed errno:" << errno << " (" << stringerror(errno) << ")");
	    }
	y2deb("poll ret:" << sel);
	if( sel>0 )
	    {
	    checkOutput();
	    }
	Wait_ii = waitpid( Pid_i, &Status_ii, WNOHANG );
	y2deb("Wait ret:" << Wait_ii);
	}
    while( Wait_ii == 0 );

    if( Wait_ii != 0 )
	{
	checkOutput();
	fclose( File_aC[IDX_STDOUT] );
	File_aC[IDX_STDOUT] = NULL;
	fclose( File_aC[IDX_STDERR] );
	File_aC[IDX_STDERR] = NULL;
	if (WIFEXITED(Status_ii))
	{
	    Ret_ir = WEXITSTATUS(Status_ii);
	    if (Ret_ir == 126)
		y2err("command \"" << lastCmd << "\" not executable");
	    else if (Ret_ir == 127)
		y2err("command \"" << lastCmd << "\" not found");
	}
	else
	{
	    Ret_ir = -127;
	    y2err("command \"" << lastCmd << "\" failed");
	}
	}

    y2deb("Wait:" << Wait_ii << " pid:" << Pid_i << " stat:" << Status_ii <<
	  " Ret:" << Ret_ir);
    return Wait_ii != 0;
    }


unsigned
SystemCmd::numLines( OutputStream Idx_iv ) const
    {
    unsigned Ret_ii;

    if( Idx_iv > 1 )
	{
	y2err("invalid index " << Idx_iv);
	}
    Ret_ii = Lines_aC[Idx_iv].size();
    y2deb("ret:" << Ret_ii);
    return Ret_ii;
    }


string
SystemCmd::getLine( unsigned Nr_iv, OutputStream Idx_iv ) const
    {
    string ret;

    if( Idx_iv > 1 )
	{
	y2err("invalid index " << Idx_iv);
	}
    if( Nr_iv < Lines_aC[Idx_iv].size() )
	{
	ret = Lines_aC[Idx_iv][Nr_iv];
	}
    return ret;
    }


void
SystemCmd::invalidate()
    {
    for (int Idx_ii = 0; Idx_ii < 2; Idx_ii++)
	{
	Lines_aC[Idx_ii].clear();
	NewLineSeen_ab[Idx_ii] = true;
	}
    }


void
SystemCmd::checkOutput()
{
    y2deb("NewLine out:" << NewLineSeen_ab[IDX_STDOUT] << " err:" << NewLineSeen_ab[IDX_STDERR]);
    if (File_aC[IDX_STDOUT])
	getUntilEOF(File_aC[IDX_STDOUT], Lines_aC[IDX_STDOUT], NewLineSeen_ab[IDX_STDOUT], false);
    if (File_aC[IDX_STDERR])
	getUntilEOF(File_aC[IDX_STDERR], Lines_aC[IDX_STDERR], NewLineSeen_ab[IDX_STDERR], true);
    y2deb("NewLine out:" << NewLineSeen_ab[IDX_STDOUT] << " err:" << NewLineSeen_ab[IDX_STDERR]);
}


#define BUF_LEN 256

void
SystemCmd::getUntilEOF(FILE* File_Cr, vector<string>& Lines_Cr, bool& NewLine_br,
		       bool Stderr_bv)
    {
    size_t old_size = Lines_Cr.size();
    char Buf_ti[BUF_LEN];
    int Cnt_ii;
    int Char_ii;
    string Text_Ci;

    clearerr( File_Cr );
    Cnt_ii = 0;
    Char_ii = EOF;
    while( (Char_ii=fgetc(File_Cr)) != EOF )
	{
	Buf_ti[Cnt_ii++] = Char_ii;
	if( Cnt_ii==sizeof(Buf_ti)-1 )
	    {
	    Buf_ti[Cnt_ii] = 0;
	    extractNewline( Buf_ti, Cnt_ii, NewLine_br, Text_Ci, Lines_Cr );
	    Cnt_ii = 0;
	    }
	Char_ii = EOF;
	}
    if( Cnt_ii>0 )
	{
	Buf_ti[Cnt_ii] = 0;
	extractNewline( Buf_ti, Cnt_ii, NewLine_br, Text_Ci, Lines_Cr );
	}
    if( Text_Ci.length() > 0 )
	{
	if( NewLine_br )
	    {
	    addLine( Text_Ci, Lines_Cr );
	    }
	else
	    {
	    Lines_Cr[Lines_Cr.size()-1] += Text_Ci;
	    }
	NewLine_br = false;
	}
    else
	{
	NewLine_br = true;
	}
    y2deb("Text_Ci:" << Text_Ci << " NewLine:" << NewLine_br);
    if( old_size != Lines_Cr.size() )
	{
	y2mil("pid:" << Pid_i << " added lines:" << Lines_Cr.size() - old_size << " stderr:" << Stderr_bv);
	}
    }


void
SystemCmd::extractNewline(const string& Buf_ti, int Cnt_iv, bool& NewLine_br,
			  string& Text_Cr, vector<string>& Lines_Cr)
    {
    string::size_type Idx_ii;

    Text_Cr += Buf_ti;
    while( (Idx_ii=Text_Cr.find( '\n' )) != string::npos )
	{
	if( !NewLine_br )
	    {
	    Lines_Cr[Lines_Cr.size()-1] += Text_Cr.substr( 0, Idx_ii );
	    }
	else
	    {
	    addLine( Text_Cr.substr( 0, Idx_ii ), Lines_Cr );
	    }
	Text_Cr.erase( 0, Idx_ii+1 );
	NewLine_br = true;
	}
    y2deb("Text_Ci:" << Text_Cr << " NewLine:" << NewLine_br);
    }


void
SystemCmd::addLine(const string& Text_Cv, vector<string>& Lines_Cr)
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
SystemCmd::logOutput() const
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


    vector<const char*>
    SystemCmd::make_env() const
    {
	vector<const char*> env;

	for (char** v = environ; *v != NULL; ++v)
	{
	    if (strncmp(*v, "LC_ALL=", strlen("LC_ALL=")) != 0 &&
		strncmp(*v, "LANGUAGE=", strlen("LANGUAGE=")) != 0)
		env.push_back(*v);
	}

	env.push_back("LC_ALL=C");
	env.push_back("LANGUAGE=C");

	env.push_back(nullptr);

	return env;
    }


string
SystemCmd::quote(const string& str)
{
    return "'" + boost::replace_all_copy(str, "'", "'\\''") + "'";
}

}
