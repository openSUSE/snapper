
#include <stdlib.h>
#include <iostream>

#include "common.h"

using namespace std;


int
main()
{
    setup();

    run_command("mkdir directory-pre");
    run_command("echo test > file-pre");
    run_command("ln --symbolic test link-pre");

    first_snapshot();

    run_command("rmdir directory-pre");
    run_command("rm file-pre");
    run_command("rm link-pre");

    run_command("mkdir directory-post");
    run_command("echo test > file-post");
    run_command("ln --symbolic test link-post");

    second_snapshot();

    check_rollback_statistics(3, 0, 3);

    rollback();

    check_rollback_errors(0, 0, 0);

    check_first();

    exit(EXIT_SUCCESS);
}
