/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

#ifndef ZYPPER_UTILS_TEXT_H_
#define ZYPPER_UTILS_TEXT_H_

#include <string>
#include <iosfwd>

/** Returns the column width of a multi-byte character string \a str */
unsigned mbs_width (const std::string & str);

/**
 * Wrap and indent given \a text and write it to the output stream \a out.
 *
 * TODO
 * - delete whitespace at the end of lines
 * - keep one-letter words with the next
 *
 * \param out       output stream to write to
 * \param test      text to wrap
 * \param indent    number of columns by which to indent the whole text
 * \param wrap_width number of columns the text should be wrapped into
 * \param initial   number of columns by which the first line should be indented
 *                  by default, the first line is indented by \a indent
 *
 */
void mbs_write_wrapped (
    std::ostream & out,
    const std::string & text,
    unsigned indent, unsigned wrap_width, int initial = -1);

/**
 * Returns a substring of a multi-byte character string \a str starting
 * at screen column \a pos and being \a n columns wide, as far as possible
 * according to the multi-column characters found in \a str.
 */
std::string mbs_substr_by_width(
    const std::string & str,
    std::string::size_type pos,
    std::string::size_type n = std::string::npos);

#endif /* ZYPPER_UTILS_TEXT_H_ */
