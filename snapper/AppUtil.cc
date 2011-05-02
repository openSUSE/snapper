/*
 * Copyright (c) [2004-2011] Novell, Inc.
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


#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <pwd.h>
#include <string>
#include <boost/algorithm/string.hpp>

#include <blocxx/AppenderLogger.hpp>
#include <blocxx/FileAppender.hpp>
#include <blocxx/Logger.hpp>
#include <blocxx/LogMessage.hpp>

#include "snapper/AppUtil.h"


namespace snapper
{
    using namespace std;


void createPath(const string& Path_Cv)
{
  string::size_type Pos_ii = 0;
  while ((Pos_ii = Path_Cv.find('/', Pos_ii + 1)) != string::npos)
    {
      string Tmp_Ci = Path_Cv.substr(0, Pos_ii);
      mkdir(Tmp_Ci.c_str(), 0777);
    }
  mkdir(Path_Cv.c_str(), 0777);
}


bool
checkDir(const string& Path_Cv)
{
  struct stat Stat_ri;

  return (stat(Path_Cv.c_str(), &Stat_ri) >= 0 &&
	  S_ISDIR(Stat_ri.st_mode));
}


bool
getStatMode(const string& Path_Cv, mode_t& val )
{
  struct stat Stat_ri;
  int ret_ii = stat(Path_Cv.c_str(), &Stat_ri);

  if( ret_ii==0 )
    val = Stat_ri.st_mode;
  else
    y2mil( "stat " << Path_Cv << " ret:" << ret_ii );

  return (ret_ii==0);
}

bool
setStatMode(const string& Path_Cv, mode_t val )
{
  int ret_ii = chmod( Path_Cv.c_str(), val );
  if( ret_ii!=0 )
    y2mil( "chmod " << Path_Cv << " ret:" << ret_ii );
  return( ret_ii==0 );
}


    bool
    getLStat(const string& path, struct stat& fs)
    {
	return lstat(path.c_str(), &fs) == 0;
    }


bool
checkNormalFile(const string& Path_Cv)
{
  struct stat Stat_ri;

  return (stat(Path_Cv.c_str(), &Stat_ri) >= 0 &&
	  S_ISREG(Stat_ri.st_mode));
}


    list<string>
    glob(const string& path, int flags)
    {
	list<string> ret;

	glob_t globbuf;
	if (glob(path.c_str(), flags, 0, &globbuf) == 0)
	{
	    for (char** p = globbuf.gl_pathv; *p != 0; p++)
		ret.push_back(*p);
	}
	globfree (&globbuf);

	return ret;
    }


    FILE*
    mkstemp(string& path)
    {
	char* tmp = strdup(path.c_str());

	int fd = ::mkstemp(tmp);
	if (fd == -1)
	    return NULL;

	path = tmp;
	free(tmp);

	return fdopen(fd, "w");
    }


static const blocxx::String component = "libsnapper";


void createLogger(const string& name, const string& logpath, const string& logfile)
{
    using namespace blocxx;

    if (logpath != "NULL" && logfile != "NULL")
    {
	String nm = name.c_str();
	LoggerConfigMap configItems;
	LogAppenderRef logApp;
	if (logpath != "STDERR" && logfile != "STDERR" &&
	    logpath != "SYSLOG" && logfile != "SYSLOG")
	{
	    String StrKey;
	    String StrPath;
	    StrKey.format("log.%s.location", name.c_str());
	    StrPath = (logpath + "/" + logfile).c_str();
	    configItems[StrKey] = StrPath;
	    logApp =
		LogAppender::createLogAppender(nm, LogAppender::ALL_COMPONENTS,
					       LogAppender::ALL_CATEGORIES,
					       "%d %-5p %c(%P) %F(%M):%L - %m",
					       LogAppender::TYPE_FILE,
					       configItems);
	}
	else if (logpath == "STDERR" && logfile == "STDERR")
	{
	    logApp =
		LogAppender::createLogAppender(nm, LogAppender::ALL_COMPONENTS,
					       LogAppender::ALL_CATEGORIES,
					       "%d %-5p %c(%P) %F(%M):%L - %m",
					       LogAppender::TYPE_STDERR,
					       configItems);
	}
	else
	{
	    logApp =
		LogAppender::createLogAppender(nm, LogAppender::ALL_COMPONENTS,
					       LogAppender::ALL_CATEGORIES,
					       "%d %-5p %c(%P) %F(%M):%L - %m",
					       LogAppender::TYPE_SYSLOG,
					       configItems);
	}

	LogAppender::setDefaultLogAppender(logApp);
    }
}


bool
testLogLevel(LogLevel level)
{
    using namespace blocxx;

    ELogLevel curLevel = LogAppender::getCurrentLogAppender()->getLogLevel();

    switch (level)
    {
	case DEBUG:
	    return curLevel >= ::blocxx::E_DEBUG_LEVEL;
	case MILESTONE:
	    return curLevel >= ::blocxx::E_INFO_LEVEL;
	case WARNING:
	    return curLevel >= ::blocxx::E_WARNING_LEVEL;
	case ERROR:
	    return curLevel >= ::blocxx::E_ERROR_LEVEL;
	default:
	    return curLevel >= ::blocxx::E_FATAL_ERROR_LEVEL;
    }
}


void
prepareLogStream(ostringstream& stream)
{
    stream.imbue(std::locale::classic());
    stream.setf(std::ios::boolalpha);
    stream.setf(std::ios::showbase);
}


ostringstream*
logStreamOpen()
{
    std::ostringstream* stream = new ostringstream;
    prepareLogStream(*stream);
    return stream;
}


void
logStreamClose(LogLevel level, const char* file, unsigned line, const char* func,
	       ostringstream* stream)
{
    using namespace blocxx;

    ELogLevel curLevel = LogAppender::getCurrentLogAppender()->getLogLevel();
    String category;

    switch (level)
    {
	case DEBUG:
	    if (curLevel >= ::blocxx::E_DEBUG_LEVEL)
		category = Logger::STR_DEBUG_CATEGORY;
	    break;
	case MILESTONE:
	    if (curLevel >= ::blocxx::E_INFO_LEVEL)
		category = Logger::STR_INFO_CATEGORY;
	    break;
	case WARNING:
	    if (curLevel >= ::blocxx::E_WARNING_LEVEL)
		category = Logger::STR_WARNING_CATEGORY;
	    break;
	case ERROR:
	    if (curLevel >= ::blocxx::E_ERROR_LEVEL)
		category = Logger::STR_ERROR_CATEGORY;
	    break;
	default:
	    if (curLevel >= ::blocxx::E_FATAL_ERROR_LEVEL)
		category = Logger::STR_FATAL_CATEGORY;
	    break;
    }

    if (!category.empty())
    {
	string tmp = stream->str();

	string::size_type pos1 = 0;

	while (true)
	{
	    string::size_type pos2 = tmp.find('\n', pos1);

	    if (pos2 != string::npos || pos1 != tmp.length())
		LogAppender::getCurrentLogAppender()->logMessage(LogMessage(component, category,
									    String(tmp.substr(pos1, pos2 - pos1)),
									    file, line, func));

	    if (pos2 == string::npos)
		break;

	    pos1 = pos2 + 1;
	}
    }

    delete stream;
}

void initDefaultLogger()
    {
    string path;
    string file;
    if (geteuid ())
	{
	struct passwd* pw = getpwuid (geteuid ());
	if (pw)
	    {
	    path = pw->pw_dir;
	    file = "snapper.log";
	    }
	else
	    {
	    path = "/";
	    file = "snapper.log";
	    }
	}
    else
	{
	path = "/var/log";
	file = "snapper.log";
	}
    createLogger("default", path, file);
    }


    int
    clonefile(const string& dest, const string& src, mode_t mode)
    {
	int src_fd = open(src.c_str(), O_RDONLY | O_LARGEFILE);

	int dest_fd = open(dest.c_str(), O_WRONLY | O_LARGEFILE | O_CREAT | O_TRUNC, mode);

#undef BTRFS_IOCTL_MAGIC
#define BTRFS_IOCTL_MAGIC 0x94
#undef BTRFS_IOC_CLONE
#define BTRFS_IOC_CLONE _IOW (BTRFS_IOCTL_MAGIC, 9, int)

	int ret = ioctl(dest_fd, BTRFS_IOC_CLONE, src_fd);

	close(dest_fd);

	close(src_fd);

	return ret;
    }


    int
    readlink(const string& path, string& buf)
    {
	char tmp[1024];
	int ret = ::readlink(path.c_str(), tmp, sizeof(tmp));
	if (ret >= 0)
	    buf = string(tmp, ret);
	return ret;
    }


    int
    symlink(const string& oldpath, const string& newpath)
    {
	return ::symlink(oldpath.c_str(), newpath.c_str());
    }


    string
    sformat(const string& format, ...)
    {
	char* result;

	va_list ap;
	va_start(ap, format);
	if (vasprintf(&result, format.c_str(), ap) == -1)
	    return string();
	va_end(ap);

	string str(result);
	free(result);
	return str;
    }


    string _(const char* msgid)
    {
	return dgettext("snapper", msgid);
    }

    string _(const char* msgid, const char* msgid_plural, unsigned long int n)
    {
	return dngettext("snapper", msgid, msgid_plural, n);
    }


    string
    hostname()
    {
	struct utsname buf;
	if (uname(&buf) != 0)
	    return string("unknown");
	string hostname(buf.nodename);
	if (strlen(buf.domainname) > 0)
	    hostname += "." + string(buf.domainname);
	return hostname;
    }


    string
    datetime(time_t t1, bool utc, bool classic)
    {
	struct tm t2;
	utc ? gmtime_r(&t1, &t2) : localtime_r(&t1, &t2);
	char buf[64 + 1];
	if (strftime(buf, sizeof(buf), classic ? "%F %T" : "%c", &t2) == 0)
	    return string("unknown");
	return string(buf);
    }


    time_t
    scan_datetime(const string& str, bool utc)
    {
	struct tm s;
	memset(&s, 0, sizeof(s));
	const char* p = strptime(str.c_str(), "%F %T", &s);
	if (!p || *p != '\0')
	    return (time_t)(-1);
	return utc ? timegm(&s) : timelocal(&s);
    }


    StopWatch::StopWatch()
    {
	gettimeofday(&start_tv, NULL);
    }


    std::ostream& operator<<(std::ostream& s, const StopWatch& sw)
    {
	struct timeval stop_tv;
	gettimeofday(&stop_tv, NULL);

	struct timeval tv;
	timersub(&stop_tv, &sw.start_tv, &tv);

	return s << fixed << double(tv.tv_sec) + (double)(tv.tv_usec) / 1000000.0 << "s";
    }

const string app_ws = " \t\n";

}
