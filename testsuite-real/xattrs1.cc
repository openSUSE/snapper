
#include <stdlib.h>
#include <iostream>

#include "common.h"
#include "xattrs_utils.h"

using namespace std;

int
main()
{
    setup();

    run_command("touch file1");
    run_command("setfacl -b file1");
    run_command("setfacl -m u:nobody:rw file1");
    xattr_create("user.aaa", "aaa-value", "/testsuite/file1");
    xattr_create("user.bbb", "bbb-value", "/testsuite/file1");

    first_snapshot();

    run_command("setfacl -b file1");
    xattr_remove("user.aaa","/testsuite/file1");
    xattr_replace("user.bbb", "bbb-new-value", "/testsuite/file1");
    xattr_create("user.ccc", "ccc-value", "/testsuite/file1");

    second_snapshot();

    undo();

    check_undo_statistics(0, 1, 0);

    check_xa_undo_statistics(2, 1, 1);

    check_undo_errors(0, 0, 0);

    check_first();

    cleanup();

    exit(EXIT_SUCCESS);
}
