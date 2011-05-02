
#include <stdlib.h>
#include <glob.h>
#include <iostream>
#include <fstream>

#include "common.h"

#include <snapper/Factory.h>
#include <snapper/Snapper.h>
#include <snapper/Snapshot.h>
#include <snapper/Comparison.h>
#include <snapper/File.h>

#include "snapper/AppUtil.h"


extern char* program_invocation_short_name;


using namespace snapper;

#define SUBVOLUME "/testsuite"

Snapper* sh = NULL;

Snapshots::iterator first;
Snapshots::iterator second;


struct CompareCallbackImpl : public CompareCallback
{
    void start() {  cout << "comparing snapshots..." << flush; }
    void stop() { cout << " done" << endl; }
};

CompareCallbackImpl compare_callback_impl;


struct RollbackProgressCallbackImpl : public RollbackProgressCallback
{
    void createInfo(const string& name) { cout << "create " << name << endl; }
    void modifyInfo(const string& name) { cout << "modify " << name << endl; }
    void deleteInfo(const string& name) { cout << "delete " << name << endl; }
};

RollbackProgressCallbackImpl rollback_progress_callback_impl;


void
setup()
{
    system("/usr/bin/find " SUBVOLUME " -mindepth 1 -maxdepth 1 -not -path " SUBVOLUME "/snapshots "
	   "-exec rm -r {} \\;");

    initDefaultLogger();

    sh = createSnapper("testsuite");

    sh->setCompareCallback(&compare_callback_impl);
    sh->setRollbackProgressCallback(&rollback_progress_callback_impl);
}


void
first_snapshot()
{
    first = sh->createPreSnapshot("testsuite");
    first->setCleanup("number");
}


void
second_snapshot()
{
    second = sh->createPostSnapshot(first);
    second->setCleanup("number");
}


void
check_rollback_statistics(unsigned int numCreate, unsigned int numModify, unsigned int numDelete)
{
    Comparison comparison(sh, first, second);

    Files& files = comparison.getFiles();
    for (Files::iterator it = files.begin(); it != files.end(); ++it)
	it->setRollback(true);

    RollbackStatistic rs = comparison.getRollbackStatistic();

    check_equal(rs.numCreate, numCreate);
    check_equal(rs.numModify, numModify);
    check_equal(rs.numDelete, numDelete);
}


void
rollback()
{
    Comparison comparison(sh, first, second);

    Files& files = comparison.getFiles();
    for (Files::iterator it = files.begin(); it != files.end(); ++it)
	it->setRollback(true);

    comparison.doRollback();
}


void
check_first()
{
    Snapshots::const_iterator current = sh->getSnapshotCurrent();

    Comparison comparison(sh, first, current);

    const Files& files = comparison.getFiles();
    for (Files::const_iterator it = files.begin(); it != files.end(); ++it)
	cout << *it << endl;

    check_true(files.empty());
}


void
run_command(const char* command)
{
    string tmp = string("cd " SUBVOLUME " ; ") + command;
    check_zero(system(tmp.c_str()));
}
