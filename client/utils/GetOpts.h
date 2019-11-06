#ifndef GET_OPTS_H
#define GET_OPTS_H

#include <getopt.h>
#include <string>
#include <map>
#include <list>


class GetOpts
{
public:

    static const struct option no_options[1];

    typedef std::map<std::string, std::string> parsed_opts;

    void init(int argc, char** argv);

    // longopts.flag must be NULL
    parsed_opts parse(const struct option* longopts);
    parsed_opts parse(const char* command, const struct option* longopts);

    bool hasArgs() const { return argc - optind > 0; }

    int numArgs() const { return argc - optind; }

    const char* popArg() { return argv[optind++]; }

    std::list<std::string> getArgs() const {
	return std::list<std::string>(&argv[optind], &argv[argc]);
    }

private:

    int argc;
    char** argv;

    std::string make_optstring(const struct option* longopts) const;

    typedef std::map<int, const char*> short2long_t;

    short2long_t make_short2long(const struct option* longopts) const;

};

#endif
