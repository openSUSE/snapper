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


#include <assert.h>
#include <unistd.h>
#include <fstream>

#include "snapper/AppUtil.h"
#include "snapper/AsciiFile.h"
#include "snapper/SnapperTypes.h"


namespace snapper
{
    using namespace std;


AsciiFile::AsciiFile(const char* Name_Cv, bool remove_empty)
    : Name_C(Name_Cv),
      remove_empty(remove_empty)
{
    reload();
}


AsciiFile::AsciiFile(const string& Name_Cv, bool remove_empty)
    : Name_C(Name_Cv),
      remove_empty(remove_empty)
{
    reload();
}


bool
AsciiFile::reload()
{
    if (Name_C.empty())
    {
	y2err("trying to load nameless AsciiFile");
	return false;
    }

    y2mil("loading file " << Name_C);
    clear();

    ifstream File_Ci(Name_C.c_str());
    classic(File_Ci);
    string Line_Ci;

    bool Ret_bi = File_Ci.good();
    File_Ci.unsetf(ifstream::skipws);
    getline( File_Ci, Line_Ci );
    while( File_Ci.good() )
    {
	Lines_C.push_back( Line_Ci );
	getline( File_Ci, Line_Ci );
    }
    return Ret_bi;
}


bool
AsciiFile::save()
{
    if (Name_C.empty())
    {
	y2err("trying to save nameless AsciiFile");
	return false;
    }

    if (remove_empty && Lines_C.empty())
    {
	y2mil("deleting file " << Name_C);

	if (access(Name_C.c_str(), F_OK) != 0)
	    return true;

	return unlink(Name_C.c_str()) == 0;
    }
    else
    {
	y2mil("saving file " << Name_C);

	ofstream file( Name_C.c_str() );
	classic(file);

	for (vector<string>::const_iterator it = Lines_C.begin(); it != Lines_C.end(); ++it)
	    file << *it << std::endl;

	file.close();

	return file.good();
    }
}


    void
    AsciiFile::logContent() const
    {
	y2mil("content of " << (Name_C.empty() ? "<nameless>" : Name_C));
	for (vector<string>::const_iterator it = Lines_C.begin(); it != Lines_C.end(); ++it)
	    y2mil(*it);
    }


void AsciiFile::append( const string& Line_Cv )
    {
    string::size_type Idx_ii;
    string Line_Ci = Line_Cv;

    removeLastIf( Line_Ci, '\n' );
    while( (Idx_ii=Line_Ci.find( '\n' ))!= string::npos )
	{
	Lines_C.push_back( Line_Ci.substr(0, Idx_ii ) );
	Line_Ci.erase( 0, Idx_ii+1 );
	}
    Lines_C.push_back( Line_Ci );
    }

void AsciiFile::append( const vector<string>& lines )
    {
    for( vector<string>::const_iterator i=lines.begin(); i!=lines.end(); ++i )
	append( *i );
    }

void AsciiFile::replace( unsigned int Start_iv, unsigned int Cnt_iv,
                         const string& Lines_Cv )
    {
    remove( Start_iv, Cnt_iv );
    insert( Start_iv, Lines_Cv );
    }

void AsciiFile::replace( unsigned int Start_iv, unsigned int Cnt_iv,
                         const vector<string>& lines )
    {
    remove( Start_iv, Cnt_iv );
    for( vector<string>::const_reverse_iterator i=lines.rbegin(); i!=lines.rend();
         ++i )
	insert( Start_iv, *i );
    }


void
AsciiFile::clear()
{
    Lines_C.clear();
}


void AsciiFile::remove( unsigned int Start_iv, unsigned int Cnt_iv )
    {
    Start_iv = max( 0u, Start_iv );
    if( Start_iv < Lines_C.size() )
	{
	Cnt_iv = min( Cnt_iv, (unsigned int)(Lines_C.size())-Start_iv );
	for( unsigned int I_ii=Start_iv; I_ii< Lines_C.size()-Cnt_iv; I_ii++ )
	    {
	    Lines_C[I_ii] = Lines_C[I_ii+Cnt_iv];
	    }
	for( unsigned int I_ii=0; I_ii<Cnt_iv; I_ii++ )
	    {
	    Lines_C.pop_back();
	    }
	}
    }

void AsciiFile::insert( unsigned int Before_iv, const string& Line_Cv )
    {
    unsigned int Idx_ii = Lines_C.size();
    if( Before_iv>=Idx_ii )
	{
	append( Line_Cv );
	}
    else
	{
	string Line_Ci = Line_Cv;
	unsigned int Cnt_ii;
	string::size_type Pos_ii;

	removeLastIf( Line_Ci, '\n' );
	Cnt_ii = 0;
	Pos_ii = 0;
	do
	  Cnt_ii++;
	while (Pos_ii = Line_Ci.find('\n', Pos_ii), Pos_ii != string::npos);
	for( unsigned int I_ii=0; I_ii<Cnt_ii; I_ii++ )
	    {
	    Lines_C.push_back( "" );
	    }
	while( Idx_ii>Before_iv )
	    {
	    Idx_ii--;
	    Lines_C[Idx_ii+Cnt_ii] = Lines_C[Idx_ii];
	    }
	while( (Pos_ii=Line_Ci.find( '\n' ))!= string::npos )
	    {
	    Lines_C[Idx_ii++] = Line_Ci.substr( Pos_ii );
	    Line_Ci.erase( 0, Pos_ii+1 );
	    }
	Lines_C[Idx_ii] = Line_Ci;
	}
    }

const string& AsciiFile::operator [] ( unsigned int Idx_iv ) const
    {
    assert( Idx_iv < Lines_C.size( ) );
    return Lines_C[Idx_iv];
    }

string& AsciiFile::operator [] ( unsigned int Idx_iv )
    {
    assert( Idx_iv < Lines_C.size( ) );
    return Lines_C[Idx_iv];
    }


void AsciiFile::removeLastIf (string& Text_Cr, char Char_cv) const
{
    if (Text_Cr.length() > 0 && Text_Cr[Text_Cr.length() - 1] == Char_cv)
	Text_Cr.erase(Text_Cr.length() - 1);
}


    bool
    SysconfigFile::getValue(const string& key, string& value) const
    {
	Regex rx('^' + Regex::ws + key + '=' + "(['\"]?)([^'\"]*)\\1" + Regex::ws + '$');

	if (find_if(lines(), regex_matches(rx)) == lines().end())
	    return false;

	value = rx.cap(2);
	y2mil("key:" << key << " value:" << value);
	return true;
    }

}
