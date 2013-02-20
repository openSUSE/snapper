
#include <stdlib.h>
#include <iostream>

#include "common.h"

using namespace std;


int
main()
{
    setup();

    run_command("ln --symbolic test1 link");
    run_command("chown --no-dereference nobody link");

    first_snapshot();

    run_command("rm link");
    run_command("ln --symbolic test2 link");
    run_command("chown --no-dereference nobody link");

    second_snapshot();

    check_undo_statistics(0, 1, 0);

    undo();

    check_undo_errors(0, 0, 0);

    check_first();

    cleanup();

    exit(EXIT_SUCCESS);
}
