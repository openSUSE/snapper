
#include <stdlib.h>
#include <iostream>

#include <snapper/Snapper.h>
#include <snapper/Comparison.h>
#include <snapper/File.h>

using namespace snapper;
using namespace std;

void print_snap_info( Snapshots::const_iterator s,
                      Snapshots::const_iterator e )
    {
    if( s==e )
	cout << "end iterator" << endl;
    else
	cout << s->getNum() << " " << s->getType() << " "
	     << s->getDescription() << endl;
    }

void print_smap_info( map<string, string> const& sm )
    {
    cout << "[";
    for( map<string,string>::const_iterator i=sm.begin(); i!=sm.end(); ++i )
	{
	cout << "('" << i->first << "', '" << i->second << "')";
	if( --sm.end() != i )
	    cout << ", ";
	}
    cout << "]" << endl;
    }


int
main(int argc, char** argv)
{
    Snapper* sh = NULL;

    // testing getConfigs
    list<ConfigInfo> c = Snapper::getConfigs();
    cout << c.size() << endl;
    for (list<ConfigInfo>::const_iterator it = c.begin(); it != c.end(); ++it)
	cout << it->getConfigName() << " " << it->getSubvolume() << endl;

    if( c.begin()!=c.end() )
	sh = new Snapper(c.front().getConfigName());

    if( sh )
	{
	// testing getIgnorePatterns
	vector<string> pl = sh->getIgnorePatterns();
	cout << pl.size() << endl;
	for (vector<string>::const_iterator s = pl.begin(); s!=pl.end(); ++s)
	    cout << *s << endl;

	// testing iterators over container
	Snapshots& snap = sh->getSnapshots();
	for (Snapshots::const_iterator s = snap.begin(); s!=snap.end(); ++s)
	    print_snap_info( s, snap.end() );
	cout << snap.size() << endl;

	Snapshots::const_iterator s = snap.begin();
	while( s!=snap.end() )
	    {
	    print_snap_info( s, snap.end() );
	    ++s;
	    }

	// testing find functions
	s = snap.find(3);
	print_snap_info( s, snap.end() );
	s = snap.getSnapshotCurrent();
	print_snap_info( s, snap.end() );
	s = snap.find(11);
	print_snap_info( s, snap.end() );
	Snapshots::iterator j = snap.findPre(s);
	print_snap_info( j, snap.end() );
	s = snap.find(10);
	print_snap_info( s, snap.end() );

	// testing handling description
	j = snap.findPost(s);
	print_snap_info( j, snap.end() );
	string desc = j->getDescription();
	desc += " 1";
	j->setDescription(desc);
	j = snap.findPost(s);
	print_snap_info( j, snap.end() );
	desc.erase( desc.size()-2 );
	j->setDescription(desc);

	// testing handling userdata
	j = snap.findPost(s);
	print_snap_info( j, snap.end() );
	map<string,string> sm=j->getUserdata();
	print_smap_info( sm );
	sm["key1"] = "value1";
	sm["key2"] = "value2";
	sm["key3"] = "value3";
	j->setUserdata( sm );
	map<string,string> sn=j->getUserdata();
	print_smap_info( sn );
	sn.clear();
	j->setUserdata( sn );
	print_smap_info( j->getUserdata() );

	// testing compare functionality
	j=snap.find(11);
	s=snap.findPre(j);
	Comparison cmp(sh,s,j);
	{
	Snapshots::const_iterator j1 = cmp.getSnapshot1();
	Snapshots::const_iterator j2 = cmp.getSnapshot2();
	print_snap_info( j1, snap.end() );
	print_snap_info( j2, snap.end() );
	const Files& flist = cmp.getFiles();
	cout << flist.size() << endl;
	for( Files::const_iterator f=flist.begin(); f!=flist.end(); ++f )
	    {
	    cout << f->getAbsolutePath(LOC_SYSTEM) << " "
	         << f->getAbsolutePath(LOC_PRE) << " "
	         << f->getAbsolutePath(LOC_POST) << endl;
	    }
	}

	// testing set/getUndo
	{
	Files& flist = cmp.getFiles();
	Files::iterator fi=flist.begin();
	if( fi!=flist.end() )
	    {
	    cout << boolalpha << fi->getUndo() << endl;
	    fi->setUndo(true);
	    cout << boolalpha << fi->getUndo() << endl;
	    fi->setUndo(false);
	    cout << boolalpha << fi->getUndo() << endl;
	    fi->setUndo(true);
	    cout << boolalpha << fi->getUndo() << endl;
	    }

	// testing doUndo
	if( fi!=flist.end() )
	    cout << boolalpha << fi->doUndo() << endl;
	}

	delete sh;
	}

    exit(EXIT_SUCCESS);
}
