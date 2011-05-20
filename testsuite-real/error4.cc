
#include <stdlib.h>
#include <iostream>

#include "common.h"

using namespace std;


int
main()
{
    setup();

    run_command("mkdir wrong-type");
    run_command("touch wrong-type/file");

    first_snapshot();

    run_command("rm wrong-type/file");

    second_snapshot();

    run_command("rmdir wrong-type");
    run_command("touch wrong-type");

    check_rollback_statistics(1, 0, 0);

    rollback();

    check_rollback_errors(1, 0, 0);

    exit(EXIT_SUCCESS);
}
