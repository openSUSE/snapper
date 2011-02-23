
#include <stdlib.h>

#include <snapper/AppUtil.h>
#include <snapper/Snapper.h>
#include <snapper/Factory.h>

using namespace snapper;


int
main(int argc, char** argv)
{
    if (argc != 2)
    {
	fprintf(stderr, "usage: hourly subvolume\n");
	exit(EXIT_FAILURE);
    }

    initDefaultLogger();

    string subvolume = argv[1];

    y2mil("hourly subvolume:" << subvolume);

    if (subvolume != "/")		// TODO
	exit(EXIT_SUCCESS);

    Snapper* sh = createSnapper(subvolume);

    Snapshots::iterator snap = sh->createSingleSnapshot("timeline");
    snap->setCleanup("timeline");

    deleteSnapper(sh);

    exit(EXIT_SUCCESS);
}
