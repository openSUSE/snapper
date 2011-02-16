
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

    rollback();

    check_first();

    exit(EXIT_SUCCESS);
}
