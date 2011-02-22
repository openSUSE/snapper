
#include <snapper/Snapper.h>
#include <snapper/Factory.h>

using namespace snapper;


int
main(int argc, char** argv)
{
    if (argc != 2)
    {
	fprintf(stderr, "usage: daily root\n");
	exit(EXIT_FAILURE);
    }

    string root = argv[1];

    Snapper* sh = createSnapper(root);

    sh->doCleanupAmount();
    sh->doCleanupTimeline();

    deleteSnapper(sh);

    exit(EXIT_SUCCESS);
}
