
#include <stdlib.h>
#include <iostream>

#include "common.h"

using namespace std;


int
main()
{
    setup();

    run_command("mkdir already-here");

    first_snapshot();

    run_command("rmdir already-here");

    second_snapshot();

    run_command("mkdir already-here");

    check_rollback_statistics(1, 0, 0);

    rollback();

    check_rollback_errors(0, 0, 0);

    check_first();

    exit(EXIT_SUCCESS);
}
