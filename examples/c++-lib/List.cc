
#include <cstdlib>
#include <iostream>

#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;


int
main(int argc, char** argv)
{
    Snapper snapper("root", "/");

    const Snapshots& snapshots = snapper.getSnapshots();

    for (const Snapshot& snapshot : snapshots)
	cout << snapshot << '\n';

    exit(EXIT_SUCCESS);
}
