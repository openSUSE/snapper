
#include <cstdlib>
#include <iostream>
#include <memory>

#include <snapper/Snapper.h>

using namespace snapper;
using namespace std;


int
main(int argc, char** argv)
{
    list<ConfigInfo> config_infos = Snapper::getConfigs("/");

    list<unique_ptr<Snapper>> snappers;

    for (const ConfigInfo& config_info : config_infos)
	snappers.push_back(make_unique<Snapper>(config_info.get_config_name(), "/"));

    for (const unique_ptr<Snapper>& snapper : snappers)
	cout << snapper->configName() << " " << snapper->subvolumeDir() << " "
	     << snapper->getSnapshots().size() << endl;

    exit(EXIT_SUCCESS);
}
