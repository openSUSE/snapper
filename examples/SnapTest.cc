
#include <stdlib.h>
#include <iostream>

#include <snapper/Factory.h>
#include <snapper/Snapper.h>

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

int
main(int argc, char** argv)
{
    list<ConfigInfo> c = Snapper::getConfigs();

    Snapper* sh = NULL;

    cout << c.size() << endl;
    for (list<ConfigInfo>::const_iterator it = c.begin(); it != c.end(); ++it)
	cout << it->config_name << " " << it->subvolume << endl;

    if( c.begin()!=c.end() )
	sh = new Snapper(c.front().config_name);

    if( sh )
	{
	vector<string> pl = sh->getIgnorePatterns();
	cout << pl.size() << endl;
	for (vector<string>::const_iterator s = pl.begin(); s!=pl.end(); ++s)
	    cout << *s << endl;
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
	s = snap.find(3);
	print_snap_info( s, snap.end() );
	s = snap.getSnapshotCurrent();
	print_snap_info( s, snap.end() );
	s = snap.find(8);
	print_snap_info( s, snap.end() );
	Snapshots::iterator j = snap.findPre(s);
	print_snap_info( j, snap.end() );
	s = snap.find(6);
	print_snap_info( s, snap.end() );
	j = snap.findPost(s);
	print_snap_info( j, snap.end() );
	string desc = j->getDescription();
	desc += " 1";
	j->setDescription(desc);
	j = snap.findPost(s);
	print_snap_info( j, snap.end() );

	delete sh;
	}

    exit(EXIT_SUCCESS);
}
