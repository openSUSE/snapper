
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
#include <snapper/SnapperDefines.h>
#include <snapper/Log.h>


extern char* program_invocation_short_name;


using namespace snapper;

#define SUBVOLUME "/testsuite"

Snapper* sh = NULL;

Snapshots::iterator first;
Snapshots::iterator second;

unsigned int numCreateErrors, numModifyErrors, numDeleteErrors;


void
setup()
{
    system("/usr/bin/find " SUBVOLUME " -mindepth 1 -maxdepth 1 -not -path " SUBVOLUME "/.snapshots "
	   "-exec rm -r {} \\;");

    initDefaultLogger();

    y2mil("Hello, please init my static mutex");

    sh = createSnapper("testsuite");
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
    second = sh->createPostSnapshot("testsuite", first);
    second->setCleanup("number");
}


void
check_undo_statistics(unsigned int numCreate, unsigned int numModify, unsigned int numDelete)
{
    Comparison comparison(sh, first, second);

    Files& files = comparison.getFiles();
    for (Files::iterator it = files.begin(); it != files.end(); ++it)
	it->setUndo(true);

    UndoStatistic rs = comparison.getUndoStatistic();

    check_equal(rs.numCreate, numCreate);
    check_equal(rs.numModify, numModify);
    check_equal(rs.numDelete, numDelete);
}


void
undo()
{
    numCreateErrors = numModifyErrors = numDeleteErrors = 0;

    cout << "comparing snapshots..." << flush;

    Comparison comparison(sh, first, second);

    cout << " done" << endl;

    Files& files = comparison.getFiles();
    for (Files::iterator it = files.begin(); it != files.end(); ++it)
	it->setUndo(true);

    cout << "undoing..." << endl;

    vector<UndoStep> undo_steps = comparison.getUndoSteps();
    for (vector<UndoStep>::const_iterator it = undo_steps.begin(); it != undo_steps.end(); ++it)
    {
	switch (it->action)
	{
	    case CREATE: cout << "creating " << it->name << endl; break;
	    case MODIFY: cout << "modifying " << it->name << endl; break;
	    case DELETE: cout << "deleting " << it->name << endl; break;
	}

	if (!comparison.doUndoStep(*it))
	{
	    switch (it->action)
	    {
		case CREATE: cout << "failed to create " << it->name << endl; numCreateErrors++; break;
		case MODIFY: cout << "failed to modify " << it->name << endl; numModifyErrors++; break;
		case DELETE: cout << "failed to delete " << it->name << endl; numDeleteErrors++; break;
	    }
	}
    }

    cout << "undoing done" << endl;
}


void
check_undo_errors(unsigned int numCreate, unsigned int numModify, unsigned int numDelete)
{
    check_equal(numCreateErrors, numCreate);
    check_equal(numModifyErrors, numModify);
    check_equal(numDeleteErrors, numDelete);
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
