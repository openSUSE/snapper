#include <iostream>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <boost/io/ios_state.hpp>

#include "console.h"
#include "text.h"

#include "Table.h"

using namespace std;

TableLineStyle Table::defaultStyle = Ascii;

static
const char * lines[][3] = {
  { "|", "-", "+"},		///< Ascii
  // utf 8
  { "\xE2\x94\x82", "\xE2\x94\x80", "\xE2\x94\xBC"}, ///< light
  { "\xE2\x94\x83", "\xE2\x94\x81", "\xE2\x95\x8B"}, ///< heavy
  { "\xE2\x95\x91", "\xE2\x95\x90", "\xE2\x95\xAC"}, ///< double
  { "\xE2\x94\x86", "\xE2\x94\x84", "\xE2\x94\xBC"}, ///< light 3
  { "\xE2\x94\x87", "\xE2\x94\x85", "\xE2\x94\x8B"}, ///< heavy 3
  { "\xE2\x94\x82", "\xE2\x94\x81", "\xE2\x94\xBF"}, ///< v light, h heavy
  { "\xE2\x94\x82", "\xE2\x95\x90", "\xE2\x95\xAA"}, ///< v light, h double
  { "\xE2\x94\x83", "\xE2\x94\x80", "\xE2\x95\x82"}, ///< v heavy, h light
  { "\xE2\x95\x91", "\xE2\x94\x80", "\xE2\x95\xAB"}, ///< v double, h light
  { ":", "-", "+" },                                 ///< colon separated values
};

void TableRow::add (const string& s) {
  _columns.push_back (s);
}

unsigned int TableRow::cols( void ) const {
  return _columns.size();
}

// 1st implementation: no width calculation, just tabs
void TableRow::dumbDumpTo (ostream &stream) const {
  bool seen_first = false;
  for (container::const_iterator i = _columns.begin (); i != _columns.end (); ++i) {
    if (seen_first)
      stream << '\t';
    seen_first = true;

    stream << *i;
  }
  stream << endl;
}

void TableRow::dumpTo (ostream &stream, const Table & parent) const
{
  const char * vline = parent._style != none ? lines[parent._style][0] : "";

  unsigned int ssize = 0; // string size in columns
  bool seen_first = false;
  container::const_iterator
    i = _columns.begin (),
    e = _columns.end ();

  stream << string(parent._margin, ' ');
  // current position at currently printed line
  int curpos = parent._margin;
  // whether to break the line now in order to wrap it to screen width
  bool do_wrap = false;
  for (unsigned c = 0; i != e ; ++i, ++c)
  {
    if (seen_first)
    {
      do_wrap =
        // user requested wrapping
        parent._do_wrap &&
        // table is wider than screen
        parent._width > parent._screen_width && (
        // the next table column would exceed the screen size
        curpos + (int) parent._max_width[c] + (parent._style != none ? 2 : 3) >
          parent._screen_width ||
        // or the user wishes to first break after the previous column
        parent._force_break_after == (int) (c - 1));

      if (do_wrap)
      {
        // start printing the next table columns to new line,
        // indent by 2 console columns
        stream << endl << string(parent._margin + 2, ' ');
        curpos = parent._margin + 2; // indent == 2
      }
      else
        // vertical line, padded with spaces
        stream << ' ' << vline << ' ';
    }
    else
      seen_first = true;

    // stream.width (widths[c]); // that does not work with multibyte chars
    const string & s = *i;
    ssize = mbs_width (s);
    if (ssize > parent._max_width[c])
    {
      unsigned cutby = parent._max_width[c] - 2;
      string cutstr = mbs_substr_by_width(s, 0, cutby);
      stream << cutstr << string(cutby - mbs_width(cutstr), ' ') << "->";
    }
    else
    {
      if (parent._header.align(c) == TableAlign::LEFT)
	stream << s << setw(parent._max_width[c] - ssize) << "";
      else
	stream << setw(parent._max_width[c] - ssize) << "" << s;
    }
    curpos += parent._max_width[c] + (parent._style != none ? 2 : 3);
  }
  stream << endl;
}


void
TableHeader::add(const string& s, TableAlign align)
{
  TableRow::add(s);
  _aligns.push_back(align);
}


// ----------------------( Table )---------------------------------------------

Table::Table()
  : _has_header (false)
  , _max_col (0)
  , _max_width(1, 0)
  , _width(0)
  , _style (defaultStyle)
  , _screen_width(get_screen_width())
  , _margin(0)
  , _force_break_after(-1)
  , _do_wrap(false)
{}

void Table::add (const TableRow& tr) {
  _rows.push_back (tr);
  updateColWidths (tr);
}

void Table::setHeader (const TableHeader& tr) {
  _has_header = true;
  _header = tr;
  updateColWidths (tr);
}

void Table::allowAbbrev(unsigned column) {
  if (column >= _abbrev_col.size()) {
    _abbrev_col.reserve(column + 1);
    _abbrev_col.insert(_abbrev_col.end(), column - _abbrev_col.size() + 1, false);
  }
  _abbrev_col[column] = true;
}

void Table::updateColWidths (const TableRow& tr) {
  // how much columns spearators add to the width of the table
  int sepwidth = _style == none ? 2 : 3;
  // initialize the width to -sepwidth (the first column does not have a line
  // on the left)
  _width = -sepwidth;

  TableRow::container::const_iterator
    i = tr._columns.begin (),
    e = tr._columns.end ();
  for (unsigned c = 0; i != e; ++i, ++c) {
    // ensure that _max_width[c] exists
    if (_max_col < c)
    {
      _max_col = c;
      _max_width.resize (_max_col + 1);
      _max_width[c] = 0;
    }

    unsigned &max = _max_width[c];
    unsigned cur = mbs_width (*i);

    if (max < cur)
      max = cur;

    _width += max + sepwidth;
  }
  _width += _margin * 2;
}

void Table::dumpRule (ostream &stream) const {
  const char * hline = _style != none ? lines[_style][1] : " ";
  const char * cross = _style != none ? lines[_style][2] : " ";

  bool seen_first = false;

  stream << string(_margin, ' ');
  for (unsigned c = 0; c <= _max_col; ++c) {
    if (seen_first) {
      stream << hline << cross << hline;
    }
    seen_first = true;
    // FIXME: could use fill character if hline were a (wide) character
    for (unsigned i = 0; i < _max_width[c]; ++i) {
      stream << hline;
    }
  }
  stream << endl;
}

void Table::dumpTo (ostream &stream) const {

  boost::io::ios_flags_saver ifs(stream);
  stream.width(0);

  // reset column widths for columns that can be abbreviated
  //! \todo allow abbrev of multiple columns?
  unsigned c = 0;
  for (vector<bool>::const_iterator it = _abbrev_col.begin();
      it != _abbrev_col.end() && c <= _max_col; ++it, ++c) {
    if (*it &&
        _width > _screen_width &&
        // don't resize the column to less than 3, or if the resulting table
        // would still exceed the screen width (bnc #534795)
        _max_width[c] > 3 &&
        _width - _screen_width < ((int) _max_width[c]) - 3) {
      _max_width[c] -= _width - _screen_width;
      break;
    }
  }

  if (_has_header) {
    _header.dumpTo (stream, *this);
    dumpRule (stream);
  }

  container::const_iterator
    b = _rows.begin (),
    e = _rows.end (),
    i;
  for (i = b; i != e; ++i) {
    i->dumpTo (stream, *this);
  }
}

void Table::wrap(int force_break_after)
{
  if (force_break_after >= 0)
    _force_break_after = force_break_after;
  _do_wrap = true;
}

void Table::lineStyle (TableLineStyle st) {
  if (st < _End)
    _style = st;
}

void Table::margin(unsigned margin) {
  if (margin < (unsigned) (_screen_width/2))
    _margin = margin;
  // else
  // ERR << "margin of " << margin << " is greater than half of the screen" << endl;
}

void Table::sort (unsigned by_column) {
  if (by_column > _max_col) {
    // ERR << "by_column >= _max_col (" << by_column << ">=" << _max_col << ")" << endl;
    // return;
  }

  TableRow::Less comp (by_column);
  _rows.sort (comp);
}

// Local Variables:
// c-basic-offset: 2
// End:
