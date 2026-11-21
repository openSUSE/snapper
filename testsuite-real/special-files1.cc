
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sysmacros.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

using namespace std;


void
create_socket(const char* name)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    check_true(fd >= 0);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/%s", SUBVOLUME, name);

    check_zero(bind(fd, (struct sockaddr*) &addr, sizeof(addr)));
    check_zero(close(fd));
}


void
check_device(const char* name, mode_t type, unsigned int major_number, unsigned int minor_number)
{
    struct stat fs;
    check_zero(lstat((string(SUBVOLUME "/") + name).c_str(), &fs));

    check_equal(fs.st_mode & S_IFMT, type);
    check_equal(major(fs.st_rdev), major_number);
    check_equal(minor(fs.st_rdev), minor_number);
}


int
main()
{
    setup();

    run_command("mknod character-pre c 1 3");
    run_command("mknod block-pre b 7 42");
    run_command("chmod 0640 block-pre");

    run_command("mkfifo fifo-pre");
    run_command("chown nobody:nobody fifo-pre");

    create_socket("socket-pre");
    run_command("chown nobody socket-pre");
    run_command("chmod 0600 socket-pre");

    first_snapshot();

    run_command("rm character-pre block-pre fifo-pre socket-pre");

    run_command("mknod character-post c 1 5");
    run_command("mknod block-post b 7 43");

    run_command("mkdir directory-post");
    run_command("mkfifo directory-post/fifo-post");
    create_socket("directory-post/socket-post");

    second_snapshot();

    check_undo_statistics(4, 0, 5);

    undo();

    check_undo_errors(0, 0, 0);

    check_first();

    check_device("character-pre", S_IFCHR, 1, 3);
    check_device("block-pre", S_IFBLK, 7, 42);

    cleanup();

    exit(EXIT_SUCCESS);
}
