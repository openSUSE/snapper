
#include <cstdlib>
#include <vector>
#include <iostream>

#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;


void
delete_all()
{
    Snapper snapper("testsuite", "/");

    Snapshots snapshots = snapper.getSnapshots();

    vector<Snapshots::iterator> tmp;
    for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	if (!it->isCurrent())
	    tmp.push_back(it);

    Plugins::Report report;

    for (vector<Snapshots::iterator>::iterator it = tmp.begin(); it != tmp.end(); ++it)
	snapper.deleteSnapshot(*it, report);
}


void
create_timeline()
{
    Snapper snapper("testsuite", "/");

    Plugins::Report report;

    time_t t = time(NULL) - 100 * 24*60*60;
    while (t < time(NULL))
    {
	SCD scd;
	scd.uid = getuid();
	scd.description = "testsuite";
	scd.cleanup = "timeline";

	Snapshots::iterator snapshot = snapper.createSingleSnapshot(scd, report);
	// snapshot->setDate(t);
	snapper.modifySnapshot(snapshot, scd, report);

	t += 60*60;
    }
}


int
main()
{
    delete_all();

    create_timeline();

    exit(EXIT_SUCCESS);
}
