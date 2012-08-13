
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <snapper/Log.h>
#include <snapper/AppUtil.h>
#include <snapper/Compare.h>
#include <snapper/File.h>

using namespace snapper;
using namespace std;


string tmp_name;

FILE* file = NULL;


void
terminate(int)
{
    if (file != NULL)
	fclose(file);

    if (!tmp_name.empty())
	unlink(tmp_name.c_str());

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

    initDefaultLogger();

    string path1 = argv[1];
    string path2 = argv[2];
    string output = argv[3];

    y2mil("compare-dirs path:" << path1 << " path2:" << path2 << " output:" << output);

    struct sigaction act;
    act.sa_handler = terminate;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    tmp_name = output + ".tmp-XXXXXX";

    file = mkstemp(tmp_name);

    cmpDirs(SDir(path1), SDir(path2), write_line);

    fclose(file);

    rename(tmp_name.c_str(), output.c_str());

    exit(EXIT_SUCCESS);
}
