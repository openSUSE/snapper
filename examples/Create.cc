
#include <stdlib.h>
#include <iostream>

#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    createSingleSnapshot("test");

    unsigned int pre_id = createPreSnapshot("test undo");

    createPostSnapshot(pre_id);

    listSnapshots();

    exit(EXIT_SUCCESS);
}
