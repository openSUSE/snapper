
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
create_number()
{
    Snapper snapper("testsuite", "/");

    Plugins::Report report;

    for (size_t i = 0; i < 100; ++i)
    {
	SCD scd;
	scd.uid = getuid();
	scd.description = "testsuite";
	scd.cleanup = "number";
	if (i % 5 == 0)
	    scd.userdata["important"] = "yes";

	snapper.createSingleSnapshot(scd, report);
    }
}


int
main()
{
    delete_all();

    create_number();

    exit(EXIT_SUCCESS);
}
