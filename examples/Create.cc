
#include <stdlib.h>
#include <iostream>

#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    getSnapper()->createSingleSnapshot("test");

    exit(EXIT_SUCCESS);
}
