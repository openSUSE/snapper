
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
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

    for (vector<Snapshots::iterator>::iterator it = tmp.begin(); it != tmp.end(); ++it)
	snapper.deleteSnapshot(*it);
}


void
create_timeline()
{
    Snapper snapper("testsuite", "/");

    time_t t = time(NULL) - 100 * 24*60*60;
    while (t < time(NULL))
    {
	SCD scd;
	scd.uid = getuid();
	scd.description = "testsuite";
	scd.cleanup = "timeline";

	Snapshots::iterator snapshot = snapper.createSingleSnapshot(scd);
	// snapshot->setDate(t);
	snapper.modifySnapshot(snapshot, scd);

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
