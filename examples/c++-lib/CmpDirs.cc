
#include <cstdlib>
#include <iostream>

#include <snapper/Compare.h>
#include <snapper/File.h>

using namespace snapper;
using namespace std;


void
log_it(const string& name, unsigned int status)
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

    cmpDirs(SDir(argv[1]), SDir(argv[2]), log_it);

    exit(EXIT_SUCCESS);
}
