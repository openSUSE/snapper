/*
 * Copyright (c) [2004-2015] Novell, Inc.
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


#ifndef SNAPPER_APP_UTIL_H
#define SNAPPER_APP_UTIL_H

#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sstream>
#include <locale>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <stdexcept>
#include <chrono>


namespace snapper
{
    using std::string;
    using std::list;
    using std::map;
    using std::vector;


    bool checkDir(const string& Path_Cv);

    list<string> glob(const string& path, int flags);

    bool clonefile(int src_fd, int dest_fd);
    bool copyfile(int src_fd, int dest_fd);

    ssize_t readlink(const string& path, string& buf);
    int symlink(const string& oldpath, const string& newpath);

    string realpath(const string& path);

    string prepend_root_prefix(const string& root_prefix, const string& path);

    string stringerror(int errnum);

    string dirname(const string& name);
    string basename(const string& name);


    struct MtabData
    {
	string device;
	string dir;
	string type;
	vector<string> options;
    };

    bool getMtabData(const string& mount_point, bool& found, MtabData& mtab_data);


    template<class StreamType>
    void classic(StreamType& stream)
    {
	stream.imbue(std::locale::classic());
    }


    string hostname();

    string datetime(time_t time, bool utc, bool classic);
    time_t scan_datetime(const string& str, bool utc);

    bool get_uid_username_gid(uid_t uid, string& username, gid_t& gid);
    bool get_user_uid(const char* username, uid_t& uid);
    bool get_group_gid(const char* groupname, gid_t& gid);
    vector<gid_t> getgrouplist(const char* username, gid_t gid);


    class StopWatch
    {
    public:

	StopWatch();

	double read() const;

	friend std::ostream& operator<<(std::ostream& s, const StopWatch& sw);

    protected:

	std::chrono::steady_clock::time_point start_time;

    };


    string sformat(const string& format, ...);


    struct runtime_error_with_errno : public std::runtime_error
    {
	explicit runtime_error_with_errno(const char* what_arg, int error_number)
	    : runtime_error(sformat("%s, errno:%d (%s)", what_arg, error_number,
				    stringerror(error_number).c_str())),
	      error_number(error_number)
	{}

	const int error_number;
    };

}

#endif
