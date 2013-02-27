#include <iostream>

#include "common.h"
#include "xattrs_utils.h"

using namespace std;

int
main()
{
    setup();

    run_command("mkdir first");
    run_command("touch second");
    run_command("touch delete-me");

    run_command("ln -s first first-link");

    // TODO: test on btrfs!
    // security namespace is curently supported for dirs and links on ext4
    xattr_create("security.aaa", "aaa-value", "/testsuite/first");
    xattr_create("security.bbb", "aaa-value", "/testsuite/second");
    xattr_create("security.ccc", "ccc-value", "/testsuite/first-link");
    xattr_create("user.aaa", "aaa-value", "/testsuite/delete-me");

    first_snapshot();

    // change type of 'first', preserve security.aaa xa
    run_command("rmdir first");
    run_command("touch first");
    xattr_create("security.aaa", "aaa-value", "/testsuite/first");

    // change type of 'second', remove security.bbb xa
    run_command("rm second");
    run_command("mkdir second");

    run_command("rm delete-me");

    second_snapshot();

    undo();

    check_undo_statistics(1, 2, 0);

    // NOTE: this test is about to fail on btrfs since the undochange cmd is implemented
    // differently on it

    // the both XAs, related to 'first' and 'second' have to be recreated on undochange cmd
    // the deleted file needs to have it's XA recreated as well on undochange
    check_xa_undo_statistics(3, 0, 0);

    check_undo_errors(0, 0, 0);

    check_first();

    cleanup();

    exit(EXIT_SUCCESS);
}
