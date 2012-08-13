
#include <stdlib.h>
#include <iostream>

#include <snapper/Compare.h>
#include <snapper/File.h>

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

    cmpDirs(SDir(argv[1]), SDir(argv[2]), log);

    exit(EXIT_SUCCESS);
}
