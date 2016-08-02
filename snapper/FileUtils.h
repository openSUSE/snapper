/*
 * Copyright (c) [2011-2014] Novell, Inc.
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


#ifndef SNAPPER_FILE_UTILS_H
#define SNAPPER_FILE_UTILS_H


#include <string>
#include <vector>
#include <functional>
#include <boost/thread.hpp>


namespace snapper
{
    using std::string;
    using std::vector;


    enum XaAttrsStatus {
	XA_UNKNOWN,
	XA_UNSUPPORTED,
	XA_SUPPORTED
    };

    class SelinuxLabelHandle;

    /*
     * The member functions of SDir and SFile are secure (avoid race
     * conditions, see openat(2)) by using either openat and alike functions
     * or by modifying the current working directory (e.g. mount, umount,
     * listxattr and getxattr).
     */

    class SDir
    {
    public:

	explicit SDir(const string& base_path);
	explicit SDir(const SDir& dir, const string& name);

	SDir(const SDir&);
	SDir& operator=(const SDir&);
	~SDir();

	static SDir deepopen(const SDir& dir, const string& name);

	int fd() const { return dirfd; }

	string fullname(bool with_base_path = true) const;
	string fullname(const string& name, bool with_base_path = true) const;

	// Type is not supported by all file system types, see readdir(3).
	typedef std::function<bool(unsigned char type, const char* name)> entries_pred_t;

	// The order of the result of the entries functions is undefined.
	vector<string> entries() const;
	vector<string> entries(entries_pred_t pred) const;
	vector<string> entries_recursive() const;
	vector<string> entries_recursive(entries_pred_t pred) const;

	int stat(struct stat* buf) const;

	int stat(const string& name, struct stat* buf, int flags) const;
	int open(const string& name, int flags) const;
	int open(const string& name, int flags, mode_t mode) const;
	ssize_t readlink(const string& name, string& buf) const;
	int mkdir(const string& name, mode_t mode) const;
	int unlink(const string& name, int flags) const;
	int chmod(const string& name, mode_t mode, int flags) const;
	int chown(const string& name, uid_t owner, gid_t group, int flags) const;
	int rename(const string& oldname, const string& newname) const;

	int mktemp(string& name) const;
	bool mkdtemp(string& name) const;

	bool xaSupported() const;

	ssize_t listxattr(const string& path, char* list, size_t size) const;
	ssize_t getxattr(const string& path, const char* name, void* value, size_t size) const;

	bool mount(const string& device, const string& mount_type, unsigned long mount_flags,
		   const string& mount_data) const;
	bool umount(const string& mount_point) const;

	bool fsetfilecon(const string& name, char* con) const;
	bool fsetfilecon(char* con) const;
	bool restorecon(SelinuxLabelHandle* sh) const;
	bool restorecon(const string& name, SelinuxLabelHandle* sh) const;

    private:

	XaAttrsStatus xastatus;
	void setXaStatus();

	const string base_path;
	const string path;

	int dirfd;

	static boost::mutex cwd_mutex;

    };


    class SFile
    {
    public:

	explicit SFile(const SDir& dir, const string& name);

	string fullname(bool with_base_path = true) const;

	int stat(struct stat* buf, int flags) const;
	int open(int flags) const;
	ssize_t readlink(string& buf) const;
	int chmod(mode_t mode, int flags) const;

	bool xaSupported() const;

	ssize_t listxattr(char* list, size_t size) const;
	ssize_t getxattr(const char* name, void* value, size_t size) const;

	void fsetfilecon(char* con) const;
	void restorecon(SelinuxLabelHandle* sh) const;

    private:

	const SDir& dir;
	const string name;

    };


    class TmpDir
    {

    public:

	TmpDir(SDir& base_dir, const string& name_template);
	~TmpDir();

	const string& getName() const { return name; }

	string getFullname() const;

    protected:

	SDir& base_dir;
	string name;

    };


    class TmpMount : public TmpDir
    {

    public:

	TmpMount(SDir& base_dir, const string& device, const string& name_template,
		 const string& mount_type, unsigned long mount_flags, const string& mount_data);
	~TmpMount();

    };

}


#endif
