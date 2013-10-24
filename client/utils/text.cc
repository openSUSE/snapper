/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

#include <libintl.h>
#include <cwchar>
#include <cstring>
#include <ostream>

#include "text.h"

using namespace std;

string _(const char* msgid)
{
    return dgettext("snapper", msgid);
}

string _(const char* msgid, const char* msgid_plural, unsigned long int n)
{
    return dngettext("snapper", msgid, msgid_plural, n);
}


// A non-ASCII string has 3 different lengths:
// - bytes
// - characters (non-ASCII ones have multiple bytes in UTF-8)
// - columns (Chinese characters are 2 columns wide)
// In #328918 see how confusing these leads to misalignment.

// return the number of columns in str, or -1 if there's an error
static
int mbs_width_e (const string & str)
{
  // from smpppd.src.rpm/format.cc, thanks arvin

  const char* ptr = str.c_str ();
  size_t s_bytes = str.length ();
  int s_cols = 0;
  bool in_ctrlseq = false;

  mbstate_t shift_state;
  memset (&shift_state, 0, sizeof (shift_state));

  wchar_t wc;
  size_t c_bytes;

  // mbrtowc produces one wide character from a multibyte string
  while ((c_bytes = mbrtowc (&wc, ptr, s_bytes, &shift_state)) > 0)
  {
    if (c_bytes >= (size_t) -2) // incomplete (-2) or invalid (-1) sequence
      return -1;

    // ignore the length of terminal control sequences in order
    // to compute the length of colored text correctly
    if (!in_ctrlseq && ::wcsncmp(&wc, L"\033", 1) == 0)
      in_ctrlseq = true;
    else if (in_ctrlseq && ::wcsncmp(&wc, L"m", 1) == 0)
      in_ctrlseq = false;
    else if (!in_ctrlseq)
      s_cols += ::wcwidth(wc);

    s_bytes -= c_bytes;
    ptr += c_bytes;
  }

  return s_cols;
}

unsigned mbs_width (const string& str)
{
  int c = mbs_width_e(str);
  if (c < 0)
    return str.length();        // fallback if there was an error
  else
    return (unsigned) c;
}

// ---------------------------------------------------------------------------

std::string mbs_substr_by_width(
    const std::string & str,
    std::string::size_type pos,
    std::string::size_type n)
{
  if (n == 0)
    return string();

  const char * ptr = str.c_str();
  const char * sptr = NULL;
  const char * eptr = NULL;
  size_t s_bytes = str.length();
  int s_cols = 0, s_cols_prev;
  bool in_ctrlseq = false;

  mbstate_t shift_state;
  memset (&shift_state, 0, sizeof (shift_state));

  wchar_t wc;
  size_t c_bytes;

  // mbrtowc produces one wide character from a multibyte string
  while ((c_bytes = mbrtowc (&wc, ptr, s_bytes, &shift_state)) > 0)
  {
    if (c_bytes >= (size_t) -2) // incomplete (-2) or invalid (-1) sequence
      return str.substr(pos, n); // default to normal string substr

    s_cols_prev = s_cols;
    // ignore the length of terminal control sequences in order
    // to compute the length of the string correctly
    if (!in_ctrlseq && ::wcsncmp(&wc, L"\033", 1) == 0)
      in_ctrlseq = true;
    else if (in_ctrlseq && ::wcsncmp(&wc, L"m", 1) == 0)
      in_ctrlseq = false;
    else if (!in_ctrlseq)
      s_cols += ::wcwidth(wc);

    // mark the beginning
    if (sptr == NULL && (unsigned) s_cols >= pos)
    {
      // cut at the right column, include also the current character
      if ((unsigned) s_cols_prev == pos)
        sptr = ptr;
      // current character cut into pieces, don't include it
      else
        sptr = ptr + c_bytes;
    }
    // mark the end
    if (n != string::npos && (unsigned) s_cols >= pos + n)
    {
      // cut at the right column, include also the current character
      if ((unsigned) s_cols == pos + n)
        eptr = ptr + c_bytes;
      // current character cut into pieces, don't include it
      else
        eptr = ptr;
      break;
    }

    s_bytes -= c_bytes;
    ptr += c_bytes;
  }

  if (eptr == NULL)
    eptr = ptr;

  if (eptr == sptr)
    return string();
  return string(sptr, eptr - sptr);
}

// ---------------------------------------------------------------------------

void mbs_write_wrapped(ostream & out, const string & text,
    unsigned indent, unsigned wrap_width, int initial)
{
  const char * s = text.c_str();
  size_t s_bytes = text.length();
  const char * prevwp = s;
  const char * linep = s;
  wchar_t wc;
  size_t bytes_read;
  bool in_word = false;
  bool in_ctrlseq = false;

  mbstate_t shift_state;
  memset (&shift_state, 0, sizeof (shift_state));

  unsigned col = 0;
  unsigned toindent = initial < 0 ? indent : initial;
  do
  {
    // indentation
    if (!col)
    {
      out << string(toindent, ' ');
      col = toindent;
    }

    bytes_read = mbrtowc (&wc, s, s_bytes, &shift_state);
    if (bytes_read > 0)
    {
      // ignore the length of terminal control sequences in order
      // to wrap colored text correctly
      if (!in_ctrlseq && ::wcsncmp(&wc, L"\033", 1) == 0)
        in_ctrlseq = true;
      else if (in_ctrlseq && ::wcsncmp(&wc, L"m", 1) == 0)
        in_ctrlseq = false;
      else if (!in_ctrlseq)
        col += ::wcwidth(wc);

      if (::iswspace(wc))
        in_word = false;
      //else if (::wcscmp(wc, L"\n"))
      //{
      //  if (!in_word)
      //    prevwp = s;
      //  in_word = true;
      //}
      else
      {
        if (!in_word)
          prevwp = s;
        in_word = true;
      }

      // current wc exceeded the wrap width
      if (col > wrap_width)
      {
        bool wanthyphen = false;
        // A single word is longer than the wrap width. Split the word and
        // append a hyphen at the end of the line. This won't normally happen,
        // but it can eventually happen e.g. with paths or too low screen widths.
        //! \todo make word-splitting more intelligent, e.g. if the word already
        //!       contains a hyphen, split there; do not split because
        //!       of a non-alphabet character; do not leave orphans, etc...
        if (linep == prevwp)
        {
          prevwp = s - 2;
          wanthyphen = true;
        }

        // update the size of the string to read
        s_bytes -= (prevwp - linep);
        // print the line, leave linep point to the start of the last word.
        for (; linep < prevwp; ++linep)
          out << *linep;
        if (wanthyphen)
          out << "-";
        out << endl;
        // reset column counter
        col = 0;
        toindent = indent;
        // reset original text pointer (points to current wc, not the start of the word)
        s = linep;
        // reset shift state
        ::memset (&shift_state, 0, sizeof (shift_state));
      }
      else
        s += bytes_read;
    }
    // we're at the end of the string
    else
    {
      // print the rest of the text
      for(; *linep; ++linep)
        out << *linep;
    }
  }
  while(bytes_read > 0);
}
