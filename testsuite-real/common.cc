
#include <stdlib.h>
#include <glob.h>
#include <iostream>
#include <fstream>

#include "common.h"

#include <snapper/Snapper.h>
#include <snapper/Factory.h>
#include <snapper/Snapshot.h>
#include <snapper/File.h>

#include "snapper/AppUtil.h"


extern char* program_invocation_short_name;


using namespace snapper;

#define ROOTDIR "/test"

Snapper* sh = NULL;

Snapshots::iterator first;
Snapshots::iterator second;


void
setup()
{
    system("/usr/bin/find " ROOTDIR " -mindepth 1 -maxdepth 1 -not -path " ROOTDIR "/snapshots "
	   "-exec rm -r {} \\;");

    initDefaultLogger();

    sh = createSnapper(ROOTDIR);
}


void
first_snapshot()
{
    first = sh->createPreSnapshot("testsuite");
    first->setCleanup("amount");
}


void
second_snapshot()
{
    second = sh->createPostSnapshot(first);
    second->setCleanup("amount");
}


void
check_rollback_statistics(unsigned int numCreate, unsigned int numModify, unsigned int numDelete)
{
    sh->setComparison(first, second);

    Files& files = sh->getFiles();
    for (Files::iterator it = files.begin(); it != files.end(); ++it)
	it->setRollback(true);

    RollbackStatistic rs = sh->getRollbackStatistic();

    check_equal(rs.numCreate, numCreate);
    check_equal(rs.numModify, numModify);
    check_equal(rs.numDelete, numDelete);
}


void
rollback()
{
    sh->setComparison(first, second);

    Files& files = sh->getFiles();
    for (Files::iterator it = files.begin(); it != files.end(); ++it)
	it->setRollback(true);

    sh->doRollback();
}


void
check_first()
{
    Snapshots::const_iterator current = sh->getSnapshotCurrent();

    sh->setComparison(first, current);

    const Files& files = sh->getFiles();
    for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	cout << *it << endl;

    check_true(files.empty());
}


void
run_command(const char* command)
{
    string tmp = string("cd " ROOTDIR " ; ") + command;
    check_zero(system(tmp.c_str()));
}
