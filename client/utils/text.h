/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

#ifndef ZYPPER_UTILS_TEXT_H_
#define ZYPPER_UTILS_TEXT_H_

#include <string>


namespace snapper
{

const char* _(const char* msgid) __attribute__ ((format_arg(1)));

const char* _(const char* msgid, const char* msgid_plural, unsigned long int n)
    __attribute__ ((format_arg(1))) __attribute__ ((format_arg(2)));

/** Returns the column width of a multi-byte character string \a str */
unsigned mbs_width (const std::string & str);

/**
 * Returns a substring of a multi-byte character string \a str starting
 * at screen column \a pos and being \a n columns wide, as far as possible
 * according to the multi-column characters found in \a str.
 */
std::string mbs_substr_by_width(
    const std::string & str,
    std::string::size_type pos,
    std::string::size_type n = std::string::npos);

}

#endif
