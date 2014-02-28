
#include "common.h"

using namespace std;


int
main()
{
    setup();

    run_command("echo hello > setuid1");
    run_command("chmod u+s,a+x setuid1");

    run_command("touch setuid2");
    run_command("chmod u+s,a+x setuid2");

    first_snapshot();

    run_command("echo world >> setuid1");

    run_command("chown :nobody setuid2");
    run_command("chmod u+s setuid2");

    second_snapshot();

    check_undo_statistics(0, 2, 0);

    undo();

    check_undo_errors(0, 0, 0);

    check_first();

    cleanup();

    exit(EXIT_SUCCESS);
}
