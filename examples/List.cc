
#include <stdlib.h>
#include <iostream>

#include <snapper/Factory.h>
#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    Snapper* sh = createSnapper();

    const Snapshots& snapshots = sh->getSnapshots();
    for (Snapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
    {
	cout << *it << endl;
    }

    exit(EXIT_SUCCESS);
}
