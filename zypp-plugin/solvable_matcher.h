#ifndef SOLVABLE_MATCHER_H
#define SOLVABLE_MATCHER_H

#include <iostream>
#include <vector>
#include <string>

struct SolvableMatcher {
    enum class Kind {
		     GLOB,
		     REGEX
    };
    std::string pattern;
    Kind kind;
    bool important;

    static std::ostream& log;
    SolvableMatcher(const std::string& apattern, Kind akind, bool aimportant)
	: pattern(apattern)
	, kind(akind)
	, important(aimportant)
    {}

    bool match(const std::string& solvable) const;
    static std::vector<SolvableMatcher> load_config(const std::string& cfg_filename);
};

#endif //SOLVABLE_MATCHER_H
