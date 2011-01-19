
#include <stdlib.h>
#include <iostream>

#include <snapper/Snapshot.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    snapshotlist.assertInit();

    for (vector<Snapshot>::const_iterator it = snapshotlist.begin();
	 it != snapshotlist.end(); ++it)
    {
	cout << *it << endl;
    }

    exit(EXIT_SUCCESS);
}
