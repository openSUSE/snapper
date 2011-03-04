
#include <stdlib.h>
#include <vector>
#include <iostream>

#include <snapper/Factory.h>
#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;


void
deleteAll()
{
    Snapper* sh = createSnapper("testsuite");

    Snapshots snapshots = sh->getSnapshots();

    vector<Snapshots::iterator> tmp;
    for (Snapshots::iterator it = snapshots.begin(); it != snapshots.end(); ++it)
	if (!it->isCurrent())
	    tmp.push_back(it);

    for (vector<Snapshots::iterator>::iterator it = tmp.begin(); it != tmp.end(); ++it)
	sh->deleteSnapshot(*it);

    deleteSnapper(sh);
}


int
main()
{
    deleteAll();

    Snapper* sh = createSnapper("testsuite");

    time_t t = time(NULL) - 100 * 24*60*60;
    while (t < time(NULL))
    {
	Snapshots::iterator snap = sh->createSingleSnapshot("testsuite");
	// snap->setDate(t);
	snap->setCleanup("timeline");

	t += 60*60;
    }

    deleteSnapper(sh);

    exit(EXIT_SUCCESS);
}
