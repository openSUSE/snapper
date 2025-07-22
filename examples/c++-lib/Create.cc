
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
    Snapper* sh = new Snapper("root", "/");

    SCD scd;
    scd.uid = getuid();
    scd.description = "test";
    scd.cleanup = "number";

    Plugins::Report report;

    sh->createSingleSnapshot(scd, report);

    delete sh;

    exit(EXIT_SUCCESS);
}
