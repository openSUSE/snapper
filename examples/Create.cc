
#include <stdlib.h>
#include <iostream>

#include <snapper/Snapshot.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    snapshotlist.createSingleSnapshot("test");

    exit(EXIT_SUCCESS);
}
