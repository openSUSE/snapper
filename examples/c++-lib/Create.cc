
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <iostream>

#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    Snapper* sh = new Snapper();

    sh->createSingleSnapshot(getuid(), "test", "number", map<string, string>());

    delete sh;

    exit(EXIT_SUCCESS);
}
