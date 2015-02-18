
#include <stdlib.h>
#include <iostream>

#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    Snapper* sh = new Snapper("root", "/");

    const Snapshots& snapshots = sh->getSnapshots();
    for (Snapshots::const_iterator it = snapshots.begin(); it != snapshots.end(); ++it)
    {
	cout << *it << endl;
    }

    delete sh;

    exit(EXIT_SUCCESS);
}
