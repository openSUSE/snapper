
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <snapper/Compare.h>

using namespace snapper;
using namespace std;


char* tmp_name = NULL;

FILE* file = NULL;


void
terminate(int)
{
    if (file != NULL)
	fclose(file);

    if (tmp_name != NULL)
	unlink(tmp_name);

    exit(EXIT_FAILURE);
}


void
write_line(const string& name, unsigned int status)
{
    fprintf(file, "%s %s\n", statusToString(status).c_str(), name.c_str());
}


int
main(int argc, char** argv)
{
    if (argc != 4)
    {
	fprintf(stderr, "usage: compare-dirs path1 path2 output\n");
	exit(EXIT_FAILURE);
    }

    daemon(0, 0);

    struct sigaction act;
    act.sa_handler = terminate;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    tmp_name = (char*) malloc(strlen(argv[3]) + 12);
    strcpy(tmp_name, argv[3]);
    strcat(tmp_name, ".tmp-XXXXXX");

    int fd = mkstemp(tmp_name);

    file = fdopen(fd, "w");

    cmpDirs(argv[1], argv[2], write_line);

    fclose(file);

    rename(tmp_name, argv[3]);

    free(tmp_name);

    exit(EXIT_SUCCESS);
}
