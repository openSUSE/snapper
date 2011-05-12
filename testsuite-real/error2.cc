
#include <stdlib.h>
#include <iostream>

#include "common.h"

using namespace std;


int
main()
{
    setup();

    run_command("touch not-here");

    first_snapshot();

    run_command("chmod a+rxw not-here");
    run_command("chown nobody not-here");

    second_snapshot();

    run_command("rm not-here");

    check_rollback_statistics(0, 1, 0);

    rollback();

    check_rollback_errors(0, 1, 0);

    exit(EXIT_SUCCESS);
}
