
#include "common.h"

using namespace std;

int
main()
{
    setup();

    run_command("touch file1");
    run_command("mkdir dir1");
    run_command("mkdir no_default");
    run_command("setfacl -b file1");
    run_command("setfacl -k dir1");
    run_command("setfacl -k no_default");
    run_command("setfacl -m u:nobody:rw file1");
    run_command("setfacl -d -m u:nobody:w dir1");

    first_snapshot();

    run_command("setfacl -b file1");
    run_command("setfacl -k dir1");

    second_snapshot();

    undo();

    check_undo_statistics(0, 2, 0);

    // do not count ACLs
    check_xa_undo_statistics(0, 0, 0);

    check_undo_errors(0, 0, 0);

    check_first();

    cleanup();

    exit(EXIT_SUCCESS);
}
