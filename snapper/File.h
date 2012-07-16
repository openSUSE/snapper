/*
 * Copyright (c) 2011 Novell, Inc.
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


#ifndef SNAPPER_FILE_H
#define SNAPPER_FILE_H


#include <sys/stat.h>

#include <string>
#include <vector>


namespace snapper
{
    using std::string;
    using std::vector;


    class Snapper;
    class Comparison;


    enum StatusFlags
    {
	CREATED = 1, DELETED = 2, TYPE = 4, CONTENT = 8, PERMISSIONS = 16, USER = 32,
	GROUP = 64
    };

    enum Cmp
    {
	CMP_PRE_TO_POST, CMP_PRE_TO_SYSTEM, CMP_POST_TO_SYSTEM
    };

    enum Location
    {
	LOC_PRE, LOC_POST, LOC_SYSTEM
    };

    enum Action
    {
	CREATE, MODIFY, DELETE
    };


    struct UndoStatistic
    {
	UndoStatistic();

	bool empty() const;

	unsigned int numCreate;
	unsigned int numModify;
	unsigned int numDelete;

	friend std::ostream& operator<<(std::ostream& s, const UndoStatistic& rs);
    };


    struct UndoStep
    {
	UndoStep(const string& name, Action action)
	    : name(name), action(action) {}

	string name;
	Action action;
    };


    class File
    {
    public:

	File(const Comparison* comparison, const string& name,
	     unsigned int pre_to_post_status)
	    : comparison(comparison), name(name), pre_to_post_status(pre_to_post_status),
	      pre_to_system_status(-1), post_to_system_status(-1), undo(false)
	{}

	const string& getName() const { return name; }

	unsigned int getPreToPostStatus() const { return pre_to_post_status; }
	unsigned int getPreToSystemStatus();
	unsigned int getPostToSystemStatus();

	unsigned int getStatus(Cmp cmp);

	string getAbsolutePath(Location loc) const;

	vector<string> getDiff(const string& options) const;

	bool getUndo() const { return undo; }
	void setUndo(bool value) { undo = value; }
	bool doUndo();

	Action getAction() const;

	friend std::ostream& operator<<(std::ostream& s, const File& file);

    private:

	const Snapper* getSnapper() const;

	bool createParentDirectories(const string& path) const;

	bool createAllTypes() const;
	bool createDirectory(mode_t mode, uid_t owner, gid_t group) const;
	bool createFile(mode_t mode, uid_t owner, gid_t group) const;
	bool createLink(uid_t owner, gid_t group) const;

	bool deleteAllTypes() const;

	bool modifyAllTypes() const;

	const Comparison* comparison;

	string name;

	unsigned int pre_to_post_status;
	unsigned int pre_to_system_status; // -1 if invalid
	unsigned int post_to_system_status; // -1 if invalid

	bool undo;

    };


    inline int operator<(const File& a, const File& b)
    {
	return a.getName() < b.getName();
    }


    class Files
    {
    public:

	friend class Comparison;

	Files(const Comparison* comparison)
	    : comparison(comparison) {}

	const Snapper* getSnapper() const;

	typedef vector<File>::iterator iterator;
	typedef vector<File>::const_iterator const_iterator;
	typedef vector<File>::size_type size_type;

	iterator begin() { return entries.begin(); }
	const_iterator begin() const { return entries.begin(); }

	iterator end() { return entries.end(); }
	const_iterator end() const { return entries.end(); }

	size_type size() const { return entries.size(); }
	bool empty() const { return entries.empty(); }

	iterator find(const string& name);
	const_iterator find(const string& name) const;

	iterator findAbsolutePath(const string& name);
	const_iterator findAbsolutePath(const string& name) const;

    private:

	void initialize();

	void create();
	bool load();
	bool save();
	void filter();

	UndoStatistic getUndoStatistic() const;

	vector<UndoStep> getUndoSteps() const;

	bool doUndoStep(const UndoStep& undo_step);

	bool doUndo();

	const Comparison* comparison;

	vector<File> entries;

    };


    string
    statusToString(unsigned int status);

    unsigned int
    stringToStatus(const string& str);

    unsigned int
    invertStatus(unsigned int status);

}


#endif
