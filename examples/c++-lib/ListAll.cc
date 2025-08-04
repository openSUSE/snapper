
#include <stdlib.h>
#include <iostream>

#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;

int
main(int argc, char** argv)
{
    list<ConfigInfo> config_infos = Snapper::getConfigs("/");

    list<Snapper*> sh;

    for (list<ConfigInfo>::const_iterator it = config_infos.begin(); it != config_infos.end(); ++it)
	sh.push_back(new Snapper(it->get_config_name(), "/"));

    for (list<Snapper*>::const_iterator it = sh.begin(); it != sh.end(); ++it)
	cout << (*it)->configName() << " " << (*it)->subvolumeDir() << " "
	     << (*it)->getSnapshots().size() << endl;

    for (list<Snapper*>::const_iterator it = sh.begin(); it != sh.end(); ++it)
	delete *it;

    exit(EXIT_SUCCESS);
}
