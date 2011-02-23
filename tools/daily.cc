
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
	fprintf(stderr, "usage: daily subvolume\n");
	exit(EXIT_FAILURE);
    }

    initDefaultLogger();

    string subvolume = argv[1];

    y2mil("daily subvolume:" << subvolume);

    Snapper* sh = createSnapper(subvolume);

    sh->doCleanupAmount();
    sh->doCleanupTimeline();

    deleteSnapper(sh);

    exit(EXIT_SUCCESS);
}
