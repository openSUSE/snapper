/*
 * Copyright (c) [2011-2013] Novell, Inc.
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


#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fnmatch.h>
#include <errno.h>
#include <fcntl.h>
#include <boost/algorithm/string.hpp>

#include "config.h"
#include "snapper/File.h"
#include "snapper/Snapper.h"
#include "snapper/AppUtil.h"
#include "snapper/Enum.h"
#include "snapper/SnapperTmpl.h"
#include "snapper/SnapperDefines.h"
#include "snapper/Compare.h"
#include "snapper/Exception.h"

#ifdef ENABLE_XATTRS
#include <snapper/XAttributes.h>
#endif

namespace snapper
{

    std::ostream& operator<<(std::ostream& s, const UndoStatistic& rs)
    {
	s << "numCreate:" << rs.numCreate
	  << " numModify:" << rs.numModify
	  << " numDelete:" << rs.numDelete;

	return s;
    }


    std::ostream& operator<<(std::ostream& s, const File& file)
    {
	s << "name:\"" << file.name << "\"";

	s << " pre_to_post_status:\"" << statusToString(file.pre_to_post_status) << "\"";

	if (file.pre_to_system_status != (unsigned int)(-1))
	    s << " pre_to_post_status:\"" << statusToString(file.pre_to_system_status) << "\"";

	if (file.post_to_system_status != (unsigned int)(-1))
	    s << " post_to_post_status:\"" << statusToString(file.post_to_system_status) << "\n";

	return s;
    }


    struct FilterHelper
    {
	FilterHelper(const vector<string>& patterns)
	    : patterns(patterns) {}
	bool operator()(const File& file)
	    {
		for (vector<string>::const_iterator it = patterns.begin(); it != patterns.end(); ++it)
		    if (fnmatch(it->c_str(), file.getName().c_str(), FNM_LEADING_DIR) == 0)
			return true;
		return false;
	    }
	const vector<string>& patterns;
    };


    void
    Files::filter(const vector<string>& ignore_patterns)
    {
	entries.erase(remove_if(entries.begin(), entries.end(), FilterHelper(ignore_patterns)),
		      entries.end());
    }


    int
    operator<(const File& a, const File& b)
    {
	return a.getName() < b.getName();
    }


    void
    Files::sort()
    {
	std::sort(entries.begin(), entries.end());
    }


    bool
    file_name_less(const File& file, const string& name)
    {
	return file.getName() < name;
    }


    Files::iterator
    Files::find(const string& name)
    {
	iterator ret = lower_bound(entries.begin(), entries.end(), name, file_name_less);
	return (ret != end() && ret->getName() == name) ? ret : end();
    }


    Files::const_iterator
    Files::find(const string& name) const
    {
	const_iterator ret = lower_bound(entries.begin(), entries.end(), name, file_name_less);
	return (ret != end() && ret->getName() == name) ? ret : end();
    }


    Files::iterator
    Files::findAbsolutePath(const string& filename)
    {
	string subvolume = file_paths->system_path;

	if (!boost::starts_with(filename, subvolume))
	    return end();

	if (subvolume == "/")
	    return find(filename);
	else
	    return find(string(filename, subvolume.size()));
    }


    Files::const_iterator
    Files::findAbsolutePath(const string& filename) const
    {
	string subvolume = file_paths->system_path;

	if (!boost::starts_with(filename, subvolume))
	    return end();

	if (subvolume == "/")
	    return find(filename);
	else
	    return find(string(filename, subvolume.size()));
    }


    unsigned int
    File::getPreToSystemStatus()
    {
	if (pre_to_system_status == (unsigned int)(-1))
	{
	    SDir dir1(file_paths->pre_path);
	    SDir dir2(file_paths->system_path);

	    string dirname = snapper::dirname(name);
	    string basename = snapper::basename(name);

	    SDir subdir1 = SDir::deepopen(dir1, dirname);
	    SDir subdir2 = SDir::deepopen(dir2, dirname);

	    pre_to_system_status = cmpFiles(SFile(subdir1, basename), SFile(subdir2, basename));
	}

	return pre_to_system_status;
    }


    unsigned int
    File::getPostToSystemStatus()
    {
	if (post_to_system_status == (unsigned int)(-1))
	{
	    SDir dir1(file_paths->post_path);
	    SDir dir2(file_paths->system_path);

	    string dirname = snapper::dirname(name);
	    string basename = snapper::basename(name);

	    SDir subdir1 = SDir::deepopen(dir1, dirname);
	    SDir subdir2 = SDir::deepopen(dir2, dirname);

	    post_to_system_status = cmpFiles(SFile(subdir1, basename), SFile(subdir2, basename));
	}

	return post_to_system_status;
    }


    unsigned int
    File::getStatus(Cmp cmp)
    {
	switch (cmp)
	{
	    case CMP_PRE_TO_POST:
		return getPreToPostStatus();

	    case CMP_PRE_TO_SYSTEM:
		return getPreToSystemStatus();

	    case CMP_POST_TO_SYSTEM:
		return getPostToSystemStatus();
	}

	return -1;
    }


    string
    File::getAbsolutePath(Location loc) const
    {
	string prefix;

	switch (loc)
	{
	    case LOC_PRE:
		prefix = file_paths->pre_path;
		break;

	    case LOC_POST:
		prefix = file_paths->post_path;
		break;

	    case LOC_SYSTEM:
		prefix = file_paths->system_path;
		break;
	}

	return prefix == "/" ? name : prefix + name;
    }


    bool
    File::createParentDirectories(const string& path) const
    {
	string::size_type pos = path.rfind('/');
	if (pos == string::npos)
	    return true;

	const string& leading_path = path.substr(0, pos);

	struct stat fs;
	if (stat(leading_path.c_str(), &fs) == 0)
	{
	    if (!S_ISDIR(fs.st_mode))
	    {
		y2err("not a directory path:" << leading_path);
		return false;
	    }

	    return true;
	}

	if (!createParentDirectories(leading_path))
	    return false;

	if (mkdir(leading_path.c_str(), 0777) != 0)
	{
	    y2err("mkdir failed path:" << leading_path << " errno:" << errno << " (" <<
		  stringerror(errno) << ")");
	    return false;
	}

	return true;
    }


    bool
    File::createAllTypes() const
    {
	struct stat fs;
	if (lstat(getAbsolutePath(LOC_PRE).c_str(), &fs) != 0)
	{
	    y2err("lstat failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno <<
		  " (" << stringerror(errno) << ")");
	    return false;
	}
	else if (!createParentDirectories(getAbsolutePath(LOC_SYSTEM)))
	{
	    return false;
	}
	else
	{
	    switch (fs.st_mode & S_IFMT)
	    {
		case S_IFDIR: {
		    if (!createDirectory(fs.st_mode, fs.st_uid, fs.st_gid))
			return false;
		} break;

		case S_IFREG: {
		    if (!createFile(fs.st_mode, fs.st_uid, fs.st_gid))
			return false;
		} break;

		case S_IFLNK: {
		    if (!createLink(fs.st_uid, fs.st_gid))
			return false;
		} break;
	    }
	}

	return true;
    }


    bool
    File::createDirectory(mode_t mode, uid_t owner, gid_t group) const
    {
	if (mkdir(getAbsolutePath(LOC_SYSTEM).c_str(), 0) != 0)
	{
	    if (errno == EEXIST && !checkDir(getAbsolutePath(LOC_SYSTEM)))
	    {
		y2err("mkdir failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno <<
		      " (" << stringerror(errno) << ")");
		return false;
	    }
	}

	if (chmod(getAbsolutePath(LOC_SYSTEM).c_str(), mode) != 0)
	{
	    y2err("chmod failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno <<
		  " (" << stringerror(errno) << ")");
	    return false;
	}

	if (chown(getAbsolutePath(LOC_SYSTEM).c_str(), owner, group) != 0)
	{
	    y2err("chown failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno <<
		  " (" << stringerror(errno) << ")");
	    return false;
	}

	return true;
    }


    bool
    File::createFile(mode_t mode, uid_t owner, gid_t group) const
    {
	int src_fd = open(getAbsolutePath(LOC_PRE).c_str(), O_RDONLY | O_LARGEFILE | O_CLOEXEC);
	if (src_fd < 0)
	{
	    y2err("open failed errno:" << errno << " (" << stringerror(errno) << ")");
	    return false;
	}

	int dest_fd = open(getAbsolutePath(LOC_SYSTEM).c_str(), O_WRONLY | O_LARGEFILE |
			   O_CREAT | O_TRUNC | O_CLOEXEC, mode);
	if (dest_fd < 0)
	{
	    y2err("open failed errno:" << errno << " (" << stringerror(errno) << ")");
	    close(src_fd);
	    return false;
	}

	int r1 = fchmod(dest_fd, mode);
	if (r1 != 0)
	{
	    y2err("fchmod failed errno:" << errno << " (" << stringerror(errno) << ")");
	    close(dest_fd);
	    close(src_fd);
	    return false;
	}

	int r2 = fchown(dest_fd, owner, group);
	if (r2 != 0)
	{
	    y2err("fchown failed errno:" << errno << " (" << stringerror(errno) << ")");
	    close(dest_fd);
	    close(src_fd);
	    return false;
	}

	bool ret = clonefile(src_fd, dest_fd) || copyfile(src_fd, dest_fd);
	if (!ret)
	{
	    y2err("clone and copy failed " << getAbsolutePath(LOC_SYSTEM));
	}

	close(dest_fd);
	close(src_fd);

	return ret;
    }


    bool
    File::createLink(uid_t owner, gid_t group) const
    {
	string tmp;
	readlink(getAbsolutePath(LOC_PRE), tmp);

	if (symlink(tmp, getAbsolutePath(LOC_SYSTEM)) != 0)
	{
	    y2err("symlink failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno <<
		  " (" << stringerror(errno) << ")");
	    return false;
	}

	if (lchown(getAbsolutePath(LOC_SYSTEM).c_str(), owner, group) != 0)
	{
	    y2err("lchown failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno <<
		  " (" << stringerror(errno) << ")");
	    return false;
	}

	return true;
    }


    bool
    File::deleteAllTypes() const
    {
	struct stat fs;
	if (lstat(getAbsolutePath(LOC_SYSTEM).c_str(), &fs) == 0)
	{
	    switch (fs.st_mode & S_IFMT)
	    {
		case S_IFDIR: {
		    if (rmdir(getAbsolutePath(LOC_SYSTEM).c_str()) != 0)
		    {
			y2err("rmdir failed path:" << getAbsolutePath(LOC_SYSTEM) <<
			      " errno:" << errno << " (" << stringerror(errno) << ")");
			return false;
		    }
		} break;

		case S_IFREG:
		case S_IFLNK: {
		    if (unlink(getAbsolutePath(LOC_SYSTEM).c_str()) != 0)
		    {
			y2err("unlink failed path:" << getAbsolutePath(LOC_SYSTEM) <<
			      " errno:" << errno << " (" << stringerror(errno) << ")");
			return false;
		    }
		} break;
	    }
	}
	else
	{
	    if (errno == ENOENT)
		return true;

	    y2err("lstat failed path:" << getAbsolutePath(LOC_SYSTEM) <<
		  " errno:" << errno << " (" << stringerror(errno) << ")");
	    return false;
	}

	return true;
    }


    bool
    File::modifyAllTypes() const
    {
	struct stat fs;
	if (lstat(getAbsolutePath(LOC_PRE).c_str(), &fs) != 0)
	{
	    y2err("lstat failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" << errno <<
		  " (" << stringerror(errno) << ")");
	    return false;
	}
	else if (!createParentDirectories(getAbsolutePath(LOC_SYSTEM)))
	{
	    return false;
	}
	else
	{
	    if (getPreToPostStatus() & CONTENT)
	    {
		switch (fs.st_mode & S_IFMT)
		{
		    case S_IFREG: {
			if (!deleteAllTypes())
			    return false;
			else if (!createFile(fs.st_mode, fs.st_uid, fs.st_gid))
			    return false;
		    } break;

		    case S_IFLNK: {
			if (!deleteAllTypes())
			    return false;
			else if (!createLink(fs.st_uid, fs.st_gid))
			    return false;
		    } break;
		}
	    }

	    if (getPreToPostStatus() & PERMISSIONS)
	    {
		if (chmod(getAbsolutePath(LOC_SYSTEM).c_str(), fs.st_mode) != 0)
		{
		    y2err("chmod failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" <<
			  errno << " (" << stringerror(errno) << ")");
		    return false;
		}
	    }

	    if (getPreToPostStatus() & (USER | GROUP))
	    {
		if (lchown(getAbsolutePath(LOC_SYSTEM).c_str(), fs.st_uid, fs.st_gid) != 0)
		{
		    y2err("lchown failed path:" << getAbsolutePath(LOC_SYSTEM) << " errno:" <<
			  errno <<  " (" << stringerror(errno) << ")");
		    return false;
		}
	    }
	}

	return true;
    }

#ifdef ENABLE_XATTRS
    bool
    File::modifyXattributes()
    {
        bool ret_val;

        try {
            XAttributes xa_src(getAbsolutePath(LOC_PRE)), xa_dest(getAbsolutePath(LOC_SYSTEM));
            y2deb("xa_src object: " << xa_src << std::endl << "xa_dest object: " << xa_dest);

            XAModification xa_mod(xa_src, xa_dest);
            y2deb("xa_modmap(xa_dest) object: " << xa_mod);

            xaCreated = xa_mod.getXaCreateNum();
            xaDeleted = xa_mod.getXaDeleteNum();
            xaReplaced = xa_mod.getXaReplaceNum();

            y2deb("xaCreated:" << xaCreated << ",xaDeleted:" << xaDeleted << ",xaReplaced:" << xaReplaced);

            ret_val = xa_mod.serializeTo(getAbsolutePath(LOC_SYSTEM));
        }
	catch (const XAttributesException& e)
	{
            ret_val = false;
        }

        return ret_val;
    }

    XAUndoStatistic& operator+=(XAUndoStatistic &out, const XAUndoStatistic &src)
    {
        out.numCreate += src.numCreate;
        out.numDelete += src.numDelete;
        out.numReplace += src.numReplace;

        return out;
    }

    XAUndoStatistic
    File::getXAUndoStatistic() const
    {
        XAUndoStatistic xs;

        xs.numCreate = xaCreated;
        xs.numDelete = xaDeleted;
        xs.numReplace = xaReplaced;

        return xs;
    }

    XAUndoStatistic
    Files::getXAUndoStatistic() const
    {
        XAUndoStatistic xs;

        for (vector<File>::const_iterator it = entries.begin(); it != entries.end(); ++it)
        {
            if (it->getUndo() && (it->getPreToPostStatus() & (DELETED | XATTRS | TYPE)))
            {
                xs += it->getXAUndoStatistic();
            }
        }

        return xs;
    }
#endif

    bool
    File::doUndo()
    {
	bool error = false;

	if (getPreToPostStatus() & CREATED || getPreToPostStatus() & TYPE)
	{
	    if (!deleteAllTypes())
		error = true;
	}

	if (getPreToPostStatus() & DELETED || getPreToPostStatus() & TYPE)
	{
	    if (!createAllTypes())
		error = true;
	}

	if (getPreToPostStatus() & (CONTENT | PERMISSIONS | USER | GROUP))
	{
	    if (!modifyAllTypes())
		error = true;
	}

#ifdef ENABLE_XATTRS
        /*
         * xattributes have to be transfered as well
         * if we'are about to create new type during
         * undo!
         */
        if (getPreToPostStatus() & (XATTRS | TYPE | DELETED))
        {
            if (!modifyXattributes())
                error = true;
        }
#endif
	pre_to_system_status = (unsigned int) -1;
	post_to_system_status = (unsigned int) -1;

	return !error;
    }


    Action
    File::getAction() const
    {
	if (getPreToPostStatus() == CREATED)
	    return DELETE;
	if (getPreToPostStatus() == DELETED)
	    return CREATE;
	return MODIFY;
    }


    UndoStatistic
    Files::getUndoStatistic() const
    {
	UndoStatistic rs;

	for (vector<File>::const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
	    if (it->getUndo())
	    {
		switch (it->getAction())
		{
		    case CREATE: rs.numCreate++; break;
		    case MODIFY: rs.numModify++; break;
		    case DELETE: rs.numDelete++; break;
		}
	    }
	}

	return rs;
    }


    vector<UndoStep>
    Files::getUndoSteps() const
    {
	vector<UndoStep> undo_steps;

	for (vector<File>::const_reverse_iterator it = entries.rbegin(); it != entries.rend(); ++it)
	{
	    if (it->getUndo())
	    {
		if (it->getPreToPostStatus() == CREATED)
		    undo_steps.push_back(UndoStep(it->getName(), it->getAction()));
	    }
	}

	for (vector<File>::const_iterator it = entries.begin(); it != entries.end(); ++it)
	{
	    if (it->getUndo())
	    {
		if (it->getPreToPostStatus() != CREATED)
		    undo_steps.push_back(UndoStep(it->getName(), it->getAction()));
	    }
	}

	return undo_steps;
    }


    bool
    Files::doUndoStep(const UndoStep& undo_step)
    {
	vector<File>::iterator it = find(undo_step.name);
	if (it == end())
	    return false;

	return it->doUndo();
    }


    string
    statusToString(unsigned int status)
    {
	string ret;

	if (status & CREATED)
	    ret += "+";
	else if (status & DELETED)
	    ret += "-";
	else if (status & TYPE)
	    ret += "t";
	else if (status & CONTENT)
	    ret += "c";
	else
	    ret += ".";

	ret += status & PERMISSIONS ? "p" : ".";
	ret += status & USER ? "u" : ".";
	ret += status & GROUP ? "g" : ".";
        ret += status & XATTRS ? "x" : ".";

	return ret;
    }


    unsigned int
    stringToStatus(const string& str)
    {
	unsigned int ret = 0;

	if (str.length() >= 1)
	{
	    switch (str[0])
	    {
		case '+': ret |= CREATED; break;
		case '-': ret |= DELETED; break;
		case 't': ret |= TYPE; break;
		case 'c': ret |= CONTENT; break;
	    }
	}

	if (str.length() >= 2)
	{
	    if (str[1] == 'p')
		ret |= PERMISSIONS;
	}

	if (str.length() >= 3)
	{
	    if (str[2] == 'u')
		ret |= USER;
	}

	if (str.length() >= 4)
	{
	    if (str[3] == 'g')
		ret |= GROUP;
	}

	if (str.length() >= 5)
        {
            if (str[4] == 'x')
                ret |= XATTRS;
        }

	return ret;
    }


    unsigned int
    invertStatus(unsigned int status)
    {
	if (status & (CREATED | DELETED))
	    return status ^ (CREATED | DELETED);
	return status;
    }

}
