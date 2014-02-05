/*
 * Copyright (c) [2004-2011] Novell, Inc.
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
#include <ostream>
#include <fstream>
#include <sys/wait.h>
#include <string>
#include <boost/algorithm/string.hpp>

#include "snapper/Log.h"
#include "snapper/AppUtil.h"
#include "snapper/SystemCmd.h"


namespace snapper
{
    using namespace std;


    SystemCmd::SystemCmd(const string& Command_Cv, bool log_output)
	: Combine_b(false), log_output(log_output)
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
    Background_b = false;
    return doExecute(Cmd_Cv);
}


int
SystemCmd::executeBackground( const string& Cmd_Cv )
{
    y2mil("SystemCmd Executing (Background):\"" << Cmd_Cv << "\"");
    Background_b = true;
    return doExecute(Cmd_Cv);
}


int
SystemCmd::executeRestricted( const string& Command_Cv,
			      long unsigned MaxTimeSec, long unsigned MaxLineOut,
			      bool& ExceedTime, bool& ExceedLines )
{
    y2mil("cmd:" << Command_Cv << " MaxTime:" << MaxTimeSec << " MaxLines:" << MaxLineOut);
    ExceedTime = ExceedLines = false;
    int ret = executeBackground( Command_Cv );
    unsigned long ts = 0;
    unsigned long ls = 0;
    unsigned long start_time = time(NULL);
    while( !ExceedTime && !ExceedLines && !doWait( false, ret ) )
	{
	if( MaxTimeSec>0 )
	    {
	    ts = time(NULL)-start_time;
	    y2mil( "time used:" << ts );
	    }
	if( MaxLineOut>0 )
	    {
	    ls = numLines()+numLines(false,IDX_STDERR);
	    y2mil( "lines out:" << ls );
	    }
	ExceedTime = MaxTimeSec>0 && ts>MaxTimeSec;
	ExceedLines = MaxLineOut>0 && ls>MaxLineOut;
	sleep( 1 );
	}
    if( ExceedTime || ExceedLines )
	{
	int r = kill( Pid_i, SIGKILL );
	y2mil( "kill pid:" << Pid_i << " ret:" << r );
	unsigned count=0;
	int Status_ii;
	int Wait_ii = -1;
	while( count<5 && Wait_ii<=0 )
	    {
	    Wait_ii = waitpid( Pid_i, &Status_ii, WNOHANG );
	    y2mil( "waitpid:" << Wait_ii );
	    count++;
	    sleep( 1 );
	    }
	/*
	r = kill( Pid_i, SIGKILL );
	y2mil( "kill pid:" << Pid_i << " ret:" << r );
	count=0;
	waitDone = false;
	while( count<8 && !waitDone )
	    {
	    y2mil( "doWait:" << count );
	    waitDone = doWait( false, ret );
	    count++;
	    sleep( 1 );
	    }
	*/
	Ret_i = -257;
	}
    else
	Ret_i = ret;
    y2mil("ret:" << ret << " ExceedTime:" << ExceedTime << " ExceedLines:" << ExceedLines);
    return ret;
}


#define PRIMARY_SHELL "/bin/sh"
#define ALTERNATE_SHELL "/bin/bash"

int
SystemCmd::doExecute( const string& Cmd )
    {
    string Shell_Ci = PRIMARY_SHELL;
    if( access( Shell_Ci.c_str(), X_OK ) != 0 )
	{
	Shell_Ci = ALTERNATE_SHELL;
	}

    lastCmd = Cmd;
    y2deb("Cmd:" << Cmd);

    StopWatch stopwatch;

    File_aC[IDX_STDERR] = File_aC[IDX_STDOUT] = NULL;
    invalidate();
    int sout[2];
    int serr[2];
    bool ok_bi = true;
    if( !testmode && pipe(sout)<0 )
	{
	y2err("pipe stdout creation failed errno:" << errno << " (" << stringerror(errno) << ")");
	ok_bi = false;
	}
    if( !testmode && !Combine_b && pipe(serr)<0 )
	{
	y2err("pipe stderr creation failed errno:" << errno << " (" << stringerror(errno) << ")");
	ok_bi = false;
	}
    if( !testmode && ok_bi )
	{
	pfds[0].fd = sout[0];
	if( fcntl( pfds[0].fd, F_SETFL, O_NONBLOCK )<0 )
	    {
	    y2err("fcntl O_NONBLOCK failed errno:" << errno << " (" << stringerror(errno) << ")");
	    }
	if( !Combine_b )
	    {
	    pfds[1].fd = serr[0];
	    if( fcntl( pfds[1].fd, F_SETFL, O_NONBLOCK )<0 )
		{
		y2err("fcntl O_NONBLOCK failed errno:" << errno << " (" << stringerror(errno) << ")");
		}
	    }
	y2deb("sout:" << pfds[0].fd << " serr:" << (Combine_b?-1:pfds[1].fd));
	switch( (Pid_i=fork()) )
	    {
	    case 0:
		setenv( "LC_ALL", "C", 1 );
		setenv( "LANGUAGE", "C", 1 );
		if( dup2( sout[1], STDOUT_FILENO )<0 )
		    {
		    y2err("dup2 stdout child failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		if( !Combine_b && dup2( serr[1], STDERR_FILENO )<0 )
		    {
		    y2err("dup2 stderr child failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		if( Combine_b && dup2( STDOUT_FILENO, STDERR_FILENO )<0 )
		    {
		    y2err("dup2 stderr child failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		if( close( sout[0] )<0 )
		    {
		    y2err("close child failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		if( !Combine_b && close( serr[0] )<0 )
		    {
		    y2err("close child failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		closeOpenFds();
		Ret_i = execl( Shell_Ci.c_str(), Shell_Ci.c_str(), "-c",
			       Cmd.c_str(), NULL );
		y2err("SHOULD NOT HAPPEN \"" << Shell_Ci << "\" Ret:" << Ret_i);
		break;
	    case -1:
		Ret_i = -1;
		break;
	    default:
		if( close( sout[1] )<0 )
		    {
		    y2err("close parent failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		if( !Combine_b && close( serr[1] )<0 )
		    {
		    y2err("close parent failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		Ret_i = 0;
		File_aC[IDX_STDOUT] = fdopen( sout[0], "r" );
		if( File_aC[IDX_STDOUT] == NULL )
		    {
		    y2err("fdopen stdout failed errno:" << errno << " (" << stringerror(errno) << ")");
		    }
		if( !Combine_b )
		    {
		    File_aC[IDX_STDERR] = fdopen( serr[0], "r" );
		    if( File_aC[IDX_STDERR] == NULL )
			{
			y2err("fdopen stderr failed errno:" << errno << " (" << stringerror(errno) << ")");
			}
		    }
		if( !Background_b )
		    {
		    doWait( true, Ret_i );
		    y2mil("stopwatch " << stopwatch << " for \"" << cmd() << "\"");
		    }
		break;
	    }
	}
    else if( !testmode )
	{
	Ret_i = -1;
	}
    else
	{
	Ret_i = 0;
	y2mil("TESTMODE would execute \"" << Cmd << "\"");
	}
    if( Ret_i==-127 || Ret_i==-1 )
	{
	y2err("system (\"" << Cmd << "\") = " << Ret_i);
	}
    if( !testmode )
	checkOutput();
    y2mil("system() Returns:" << Ret_i);
    if (Ret_i != 0 && log_output)
	logOutput();
    return Ret_i;
    }


bool
SystemCmd::doWait( bool Hang_bv, int& Ret_ir )
    {
    int Wait_ii;
    int Status_ii;

    do
	{
	y2deb("[0] id:" <<  pfds[0].fd << " ev:" << hex << (unsigned)pfds[0].events << dec << " [1] fs:" <<
	      (Combine_b?-1:pfds[1].fd) << " ev:" << hex << (Combine_b?0:(unsigned)pfds[1].events));
	int sel = poll( pfds, Combine_b?1:2, 1000 );
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
    while( Hang_bv && Wait_ii == 0 );

    if( Wait_ii != 0 )
	{
	checkOutput();
	fclose( File_aC[IDX_STDOUT] );
	File_aC[IDX_STDOUT] = NULL;
	if( !Combine_b )
	    {
	    fclose( File_aC[IDX_STDERR] );
	    File_aC[IDX_STDERR] = NULL;
	    }
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
	  " Hang:" << Hang_bv << " Ret:" << Ret_ir);
    return Wait_ii != 0;
    }


void
SystemCmd::setCombine(bool val)
{
    Combine_b = val;
}


void
SystemCmd::setTestmode(bool val)
{
    testmode = val;
}


unsigned
SystemCmd::numLines( bool Sel_bv, OutputStream Idx_iv ) const
    {
    unsigned Ret_ii;

    if( Idx_iv > 1 )
	{
	y2err("invalid index " << Idx_iv);
	}
    if( Sel_bv )
	{
	Ret_ii = SelLines_aC[Idx_iv].size();
	}
    else
	{
	Ret_ii = Lines_aC[Idx_iv].size();
	}
    y2deb("ret:" << Ret_ii);
    return Ret_ii;
    }


string
SystemCmd::getLine( unsigned Nr_iv, bool Sel_bv, OutputStream Idx_iv ) const
    {
    string ret;

    if( Idx_iv > 1 )
	{
	y2err("invalid index " << Idx_iv);
	}
    if( Sel_bv )
	{
	if( Nr_iv < SelLines_aC[Idx_iv].capacity() )
	    {
	    ret = *SelLines_aC[Idx_iv][Nr_iv];
	    }
	}
    else
	{
	if( Nr_iv < Lines_aC[Idx_iv].size() )
	    {
	    ret = Lines_aC[Idx_iv][Nr_iv];
	    }
	}
    return ret;
    }


int
SystemCmd::select( const string& Pat_Cv, OutputStream Idx_iv )
    {
    if( Idx_iv > 1 )
	{
	y2err("invalid index " << Idx_iv);
	}
    string Search_Ci( Pat_Cv );
    bool BeginOfLine_bi = Search_Ci.length()>0 && Search_Ci[0]=='^';
    if( BeginOfLine_bi )
	{
	Search_Ci.erase( 0, 1 );
	}
    SelLines_aC[Idx_iv].resize(0);
    int Size_ii = 0;
    int End_ii = Lines_aC[Idx_iv].size();
    for( int I_ii=0; I_ii<End_ii; I_ii++ )
	{
	string::size_type Pos_ii = Lines_aC[Idx_iv][I_ii].find( Search_Ci );
	if( Pos_ii>0 && BeginOfLine_bi )
	    {
	    Pos_ii = string::npos;
	    }
	if (Pos_ii != string::npos)
	    {
	    SelLines_aC[Idx_iv].resize( Size_ii+1 );
	    SelLines_aC[Idx_iv][Size_ii] = &Lines_aC[Idx_iv][I_ii];
	    y2deb("Select Added Line " << Size_ii << " \"" << *SelLines_aC[Idx_iv][Size_ii] << "\"");
	    Size_ii++;
	    }
	}

    y2mil("Pid:" << Pid_i << " Idx:" << Idx_iv << " Pattern:\"" << Pat_Cv << "\" Lines:" << Size_ii);
    return Size_ii;
    }


void
SystemCmd::invalidate()
    {
    for (int Idx_ii = 0; Idx_ii < 2; Idx_ii++)
	{
	SelLines_aC[Idx_ii].resize(0);
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
    unsigned lines = numLines(false, IDX_STDERR);
    if (lines <= line_limit)
    {
	for (unsigned i = 0; i < lines; ++i)
	    y2mil("stderr:" << getLine(i, false, IDX_STDERR));
    }
    else
    {
	for (unsigned i = 0; i < line_limit / 2; ++i)
	    y2mil("stderr:" << getLine(i, false, IDX_STDERR));
	y2mil("stderr omitting lines");
	for (unsigned i = lines - line_limit / 2; i < lines; ++i)
	    y2mil("stderr:" << getLine(i, false, IDX_STDERR));
    }

    lines = numLines(false, IDX_STDOUT);
    if (lines <= line_limit)
    {
	for (unsigned i = 0; i < lines; ++i)
	    y2mil("stdout:" << getLine(i, false, IDX_STDOUT));
    }
    else
    {
	for (unsigned i = 0; i < line_limit / 2; ++i)
	    y2mil("stdout:" << getLine(i, false, IDX_STDOUT));
	y2mil("stdout omitting lines");
	for (unsigned i = lines - line_limit / 2; i < lines; ++i)
	    y2mil("stdout:" << getLine(i, false, IDX_STDOUT));
    }
}


string
SystemCmd::quote(const string& str)
{
    return "'" + boost::replace_all_copy(str, "'", "'\\''") + "'";
}


string
SystemCmd::quote(const list<string>& strs)
{
    string ret;
    for (std::list<string>::const_iterator it = strs.begin(); it != strs.end(); it++)
    {
	if (it != strs.begin())
	    ret.append(" ");
	ret.append(quote(*it));
    }
    return ret;
}


bool SystemCmd::testmode = false;

}
