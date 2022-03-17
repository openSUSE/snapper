
#include "common.h"
#include "xattrs-utils.h"

using namespace std;

int
main()
{
    setup();

    run_command("mkdir first");
    run_command("touch second");
    run_command("touch delete-me");

    run_command("ln -s first first-link");

    // user.* namespace is allowed only for regular files and directories (restricted by VFS)
    // security namespace is verified to work with symlinks on ext4 and btrfs
    xattr_create("security.aaa", "aaa-value", SUBVOLUME "/first");
    xattr_create("security.bbb", "aaa-value", SUBVOLUME "/second");
    xattr_create("security.ccc", "ccc-value", SUBVOLUME "/first-link");
    xattr_create("user.aaa", "aaa-value", SUBVOLUME "/delete-me");

    first_snapshot();

    // change type of 'first', preserve security.aaa xa
    run_command("rmdir first");
    run_command("touch first");
    xattr_create("security.aaa", "aaa-value", SUBVOLUME "/first");

    // change type of 'second', remove security.bbb xa
    run_command("rm second");
    run_command("mkdir second");

    run_command("rm delete-me");

    second_snapshot();

    undo();

    check_undo_statistics(1, 2, 0);

    // the both XAs, related to 'first' and 'second' have to be recreated on undochange cmd
    // the deleted file needs to have it's XA recreated as well on undochange
    check_xa_undo_statistics(3, 0, 0);

    check_undo_errors(0, 0, 0);

    check_first();

    cleanup();

    exit(EXIT_SUCCESS);
}
