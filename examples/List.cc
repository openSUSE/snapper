
#include <stdlib.h>
#include <iostream>

#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    const Snapshots& snapshots = getSnapper()->getSnapshots();
    for (Snapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
    {
	cout << *it << endl;
    }

    exit(EXIT_SUCCESS);
}
