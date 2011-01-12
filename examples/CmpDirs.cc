
#include <stdlib.h>
#include <iostream>

#include <snapper/Files.h>

using namespace snapper;
using namespace std;


void
log(const string& name, unsigned int status)
{
    cout << statusToString(status) << " " << name << endl;
}


int
main(int argc, char** argv)
{
    if (argc != 3)
    {
	cerr << "usage: CmdDirs path1 path2\n";
	exit(EXIT_FAILURE);
    }

    cmpDirs(argv[1], argv[2], log);

    exit(EXIT_SUCCESS);
}
