
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

    sh->createSingleSnapshot("test");

    exit(EXIT_SUCCESS);
}
