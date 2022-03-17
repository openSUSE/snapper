
#include "common.h"
#include "xattrs-utils.h"

using namespace std;

int
main()
{
    setup();

    run_command("touch foo");
    run_command("touch bar");

    xattr_create("user.empty", "", SUBVOLUME "/foo");
    xattr_create("user.empty", "not-yet", SUBVOLUME "/bar");

    first_snapshot();

    xattr_replace("user.empty", "not-anymore", SUBVOLUME "/foo");
    xattr_replace("user.empty", "", SUBVOLUME "/bar");

    second_snapshot();

    undo();

    check_undo_statistics(0, 2, 0);

    check_xa_undo_statistics(0, 2, 0);

    check_undo_errors(0, 0, 0);

    check_first();

    cleanup();

    exit(EXIT_SUCCESS);
}
