
#include <stdlib.h>
#include <iostream>

#include "common.h"

using namespace std;


int
main()
{
    setup();

    run_command("mkdir --mode a=rwx directory");

    run_command("echo test > file");
    run_command("chmod a=rwx file");

    first_snapshot();

    run_command("chmod a=--- directory");
    run_command("chmod a=--- file");

    second_snapshot();

    check_undo_statistics(0, 2, 0);

    undo();

    check_undo_errors(0, 0, 0);

    check_first();

    exit(EXIT_SUCCESS);
}
