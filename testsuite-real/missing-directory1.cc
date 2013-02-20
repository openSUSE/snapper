
#include <stdlib.h>
#include <iostream>

#include "common.h"

using namespace std;


int
main()
{
    setup();

    run_command("mkdir not-here");
    run_command("touch not-here/file");
    run_command("mkdir not-here/directory");

    first_snapshot();

    run_command("rm not-here/file");
    run_command("rmdir not-here/directory");

    second_snapshot();

    run_command("rmdir not-here");

    check_undo_statistics(2, 0, 0);

    undo();

    check_undo_errors(0, 0, 0);

    check_first();

    cleanup();

    exit(EXIT_SUCCESS);
}
