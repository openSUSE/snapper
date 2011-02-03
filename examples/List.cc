
#include <stdlib.h>
#include <iostream>

#include <snapper/Snapshot.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    snapshots.assertInit();

    for (list<Snapshot>::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
    {
	cout << *it << endl;
    }

    exit(EXIT_SUCCESS);
}
