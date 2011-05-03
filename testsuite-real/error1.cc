
#include <stdlib.h>
#include <iostream>

#include "common.h"

using namespace std;


int
main()
{
    setup();

    first_snapshot();

    run_command("mkdir not-empty");

    second_snapshot();

    run_command("touch not-empty/bad");

    check_rollback_statistics(0, 0, 1);

    rollback();

    check_rollback_errors(0, 0, 1);

    exit(EXIT_SUCCESS);
}
