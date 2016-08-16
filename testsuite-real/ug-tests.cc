
#include <unistd.h>
#include <sys/types.h>

#include <iostream>

#include <snapper/AppUtil.h>

using namespace std;
using namespace snapper;


void
test1()
{
    uid_t uid = getuid();
    cout << "uid:" << uid << endl;

    string username;
    gid_t gid;
    if (!get_uid_username_gid(uid, username, gid))
	cerr << "failed to get username and gid" << endl;
    cout << "username:" << username << endl;
    cout << "gid:" << gid << endl;

    vector<gid_t> gids = getgrouplist(username.c_str(), gid);
    cout << "gids:";
    for (gid_t gid : gids)
	cout << gid << " ";
    cout << endl;

    cout << endl;
}


void
test2()
{
    uid_t uid;
    if (!get_user_uid("root", uid))
	cerr << "failed to get uid" << endl;
    cout << "uid:" << uid << endl;

    gid_t gid;
    if (!get_group_gid("audio", gid))
	cerr << "failed to get gid" << endl;
    cout << "gid:" << gid << endl;

    cout << endl;
}


int
main()
{
    test1();
    test2();
}
