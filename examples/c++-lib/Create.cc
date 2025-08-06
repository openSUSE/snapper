
#include <cstdlib>
#include <iostream>

#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;


int
main(int argc, char** argv)
{
    Snapper snapper("root", "/");

    SCD scd;
    scd.uid = getuid();
    scd.description = "test";
    scd.cleanup = "number";

    Plugins::Report report;

    snapper.createSingleSnapshot(scd, report);

    exit(EXIT_SUCCESS);
}
