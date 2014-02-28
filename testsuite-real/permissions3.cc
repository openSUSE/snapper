
#include "common.h"

using namespace std;


int
main()
{
    setup();

    run_command("echo hello > setuid");
    run_command("chmod u+s setuid");

    first_snapshot();

    run_command("echo world >> setuid");

    second_snapshot();

    check_undo_statistics(0, 1, 0);

    undo();

    check_undo_errors(0, 0, 0);

    check_first();

    cleanup();

    exit(EXIT_SUCCESS);
}
