
#include <stdlib.h>
#include <iostream>

#include <snapper/Factory.h>
#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    list<ConfigInfo> c = Snapper::getConfigs();

    list<Snapper*> sh;

    for (list<ConfigInfo>::const_iterator it = c.begin(); it != c.end(); ++it)
	sh.push_back(new Snapper(it->config_name));

    for (list<Snapper*>::const_iterator it = sh.begin(); it != sh.end(); ++it)
	cout << (*it)->configName() << " " << (*it)->subvolumeDir() << " "
	     << (*it)->getSnapshots().size() << endl;

    for (list<Snapper*>::const_iterator it = sh.begin(); it != sh.end(); ++it)
	delete *it;

    exit(EXIT_SUCCESS);
}
