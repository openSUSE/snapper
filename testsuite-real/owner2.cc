
#include <stdlib.h>
#include <iostream>

#include "common.h"

using namespace std;


int
main()
{
    setup();

    run_command("mkdir directory");
    run_command("echo test > file");
    run_command("ln --symbolic test link");

    first_snapshot();

    run_command("chown nobody directory");
    run_command("chown nobody file");
    run_command("chown --no-dereference nobody link");

    second_snapshot();

    check_rollback_statistics(0, 3, 0);

    rollback();

    check_first();

    exit(EXIT_SUCCESS);
}
