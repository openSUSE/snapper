/*
 * Copyright (c) [2011-2015] Novell, Inc.
 * Copyright (c) 2022 SUSE LLC
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


    enum StatusFlags
    {
	CREATED = 1,		// created
	DELETED = 2,		// deleted
	TYPE = 4,		// type has changed
	CONTENT = 8,		// content has changed
	PERMISSIONS = 16,	// permissions have changed, see chmod(2)
	OWNER = 32,		// owner has changed, see chown(2)
	GROUP = 64,		// group has changed, see chown(2)
	XATTRS = 128,		// extended attributes changed, see attr(5)
	ACL = 256		// access control list changed, see acl(5)
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
	bool empty() const { return numCreate == 0 && numModify == 0 && numDelete == 0; }

	unsigned int numCreate = 0;
	unsigned int numModify = 0;
	unsigned int numDelete = 0;

	friend std::ostream& operator<<(std::ostream& s, const UndoStatistic& rs);
    };


    struct XAUndoStatistic
    {
	bool empty() const { return numCreate == 0 && numReplace == 0 && numDelete == 0; }

	unsigned int numCreate = 0;
	unsigned int numReplace = 0;
	unsigned int numDelete = 0;

	friend XAUndoStatistic& operator+=(XAUndoStatistic&, const XAUndoStatistic&);
    };


    struct UndoStep
    {
	UndoStep(const string& name, Action action)
	    : name(name), action(action) {}

	string name;
	Action action;
    };


    struct FilePaths
    {
	string system_path;
	string pre_path;
	string post_path;
    };


    class File
    {
    public:

	File(const FilePaths* file_paths, const string& name, unsigned int pre_to_post_status)
	    : file_paths(file_paths), name(name), pre_to_post_status(pre_to_post_status)
	{}

	const string& getName() const { return name; }

	unsigned int getPreToPostStatus() const { return pre_to_post_status; }
	unsigned int getPreToSystemStatus();
	unsigned int getPostToSystemStatus();

	unsigned int getStatus(Cmp cmp);

	string getAbsolutePath(Location loc) const;

	bool getUndo() const { return undo; }
	void setUndo(bool value) { undo = value; }
	bool doUndo();

	Action getAction() const;

	friend std::ostream& operator<<(std::ostream& s, const File& file);

	XAUndoStatistic getXAUndoStatistic() const;

	// C++ locale aware less-than comparison
	static bool cmp_lt(const string& lhs, const string& rhs);

    private:

	bool createParentDirectories(const string& path) const;

	bool createAllTypes() const;
	bool createDirectory(mode_t mode, uid_t owner, gid_t group) const;
	bool createFile(mode_t mode, uid_t owner, gid_t group) const;
	bool createLink(uid_t owner, gid_t group) const;

	bool deleteAllTypes() const;

	bool modifyAllTypes() const;

	const FilePaths* file_paths;

	string name;

	unsigned int pre_to_post_status = -1;
	unsigned int pre_to_system_status = -1; // -1 if invalid
	unsigned int post_to_system_status = -1; // -1 if invalid

	bool undo = false;

	bool modifyXattributes();
	bool modifyAcls();

	unsigned int xaCreated = 0;
	unsigned int xaDeleted = 0;
	unsigned int xaReplaced = 0;

    };


    class Files
    {
    public:

	friend class Comparison;

	Files(const FilePaths* file_paths)
	    : file_paths(file_paths) {}

	Files(const FilePaths* file_paths, const vector<File>& entries)
	    : file_paths(file_paths), entries(entries) {}

	typedef vector<File>::iterator iterator;
	typedef vector<File>::const_iterator const_iterator;
	typedef vector<File>::size_type size_type;

	iterator begin() { return entries.begin(); }
	const_iterator begin() const { return entries.begin(); }

	iterator end() { return entries.end(); }
	const_iterator end() const { return entries.end(); }

	size_type size() const { return entries.size(); }
	bool empty() const { return entries.empty(); }

	void clear();

	iterator find(const string& name);
	const_iterator find(const string& name) const;

	iterator findAbsolutePath(const string& name);
	const_iterator findAbsolutePath(const string& name) const;

	UndoStatistic getUndoStatistic() const;

	vector<UndoStep> getUndoSteps() const;

	bool doUndoStep(const UndoStep& undo_step);

	XAUndoStatistic getXAUndoStatistic() const;

    protected:

	void push_back(const File& file) { entries.push_back(file); }

	void filter(const vector<string>& ignore_patterns);

	void sort();

	const FilePaths* file_paths;

    private:

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
