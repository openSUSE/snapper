/*
 * Copyright (c) [2021-2024] SUSE LLC
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
 * with this program; if not, contact SUSE LLC.
 *
 * To contact SUSE LLC about this file by physical or electronic mail, you may
 * find current contact information at www.suse.com.
 */


#include <langinfo.h>
#include <cstring>

#include "Table.h"
#include "text.h"
#include "console.h"


namespace snapper
{

    using namespace std;


    struct Table::OutputInfo
    {
	OutputInfo(const Table& table);

	void calculate_hidden(const Table& table, const Table::Row& row);
	void calculate_trims(const Table& table, const Table::Row& row);
	void calculate_widths(const Table& table, const Table::Row& row, bool is_header, unsigned indent);
	size_t calculate_total_width(const Table& table) const;
	void calculate_abbriviated_widths(const Table& table);

	string trimmed(const string& s, Align align, size_t trim) const;

	struct ColumnVars
	{
	    bool hidden = false;
	    size_t width = 0;
	    size_t trim = -1;
	};

	vector<ColumnVars> column_vars;
    };


    Table::OutputInfo::OutputInfo(const Table& table)
    {
	if (table.column_params.size() != table.header.get_columns().size())
	    throw runtime_error("column_params.size != header.size");

	column_vars.resize(table.header.get_columns().size());

	// calculate hidden, default to false

	for (size_t idx = 0; idx < table.column_params.size(); ++idx)
	{
	    if (table.column_params[idx].visibility == Visibility::AUTO || table.column_params[idx].visibility == Visibility::OFF)
		column_vars[idx].hidden = true;
	}

	for (const Table::Row& row : table.rows)
	    calculate_hidden(table, row);

	// calculate trims

	for (const Table::Row& row : table.rows)
	    calculate_trims(table, row);

	// calculate widths

	for (size_t idx = 0; idx < table.column_params.size(); ++idx)
	    column_vars[idx].width = table.column_params[idx].min_width;

	if (table.show_header)
	    calculate_widths(table, table.header, true, 0);

	for (const Table::Row& row : table.rows)
	    calculate_widths(table, row, false, 0);

	calculate_abbriviated_widths(table);
    }


    void
    Table::OutputInfo::calculate_hidden(const Table& table, const Table::Row& row)
    {
	const vector<string>& columns = row.get_columns();

	if (columns.size() > table.column_params.size())
	    throw runtime_error("too many columns");

	for (size_t idx = 0; idx < columns.size(); ++idx)
	{
	    if (table.column_params[idx].visibility == Visibility::AUTO && !columns[idx].empty())
		column_vars[idx].hidden = false;
	}

	for (const Table::Row& subrow : row.get_subrows())
	    calculate_hidden(table, subrow);
    }


    void
    Table::OutputInfo::calculate_trims(const Table& table, const Table::Row& row)
    {
	const vector<string>& columns = row.get_columns();

	for (size_t idx = 0; idx < columns.size(); ++idx)
	{
	    if (!table.column_params[idx].trim || column_vars[idx].hidden)
		continue;

	    if (column_vars[idx].trim == 0)
		continue;

	    const string& column = columns[idx];
	    if (column.empty())
		continue;

	    size_t trim = 0;

	    switch (table.column_params[idx].align)
	    {
		case Align::RIGHT:
		    trim = column.size() - column.find_last_not_of(" ") - 1;
		    break;

		case Align::LEFT:
		    trim = column.find_first_not_of(" ");
		    break;
	    }

	    column_vars[idx].trim = min(column_vars[idx].trim, trim);
	}

	for (const Table::Row& subrow : row.get_subrows())
	    calculate_trims(table, subrow);
    }


    string
    Table::OutputInfo::trimmed(const string& s, Align align, size_t trim) const
    {
	string ret = s;

	switch (align)
	{
	    case Align::RIGHT:
		if (ret.size() >= trim)
		    ret.erase(ret.size() - trim, trim);
		break;

	    case Align::LEFT:
		if (ret.size() >= trim)
		    ret.erase(0, trim);
		break;
	}

	return ret;
    }


    void
    Table::OutputInfo::calculate_widths(const Table& table, const Table::Row& row, bool is_header, unsigned indent)
    {
	const vector<string>& columns = row.get_columns();

	for (size_t idx = 0; idx < columns.size(); ++idx)
	{
	    if (column_vars[idx].hidden)
		continue;

	    string column = columns[idx];

	    if (!is_header && table.column_params[idx].trim)
		column = trimmed(column, table.column_params[idx].align, column_vars[idx].trim);

	    size_t width = mbs_width(column);

	    if (idx == table.tree_idx)
		width += 2 * indent;

	    column_vars[idx].width = max(column_vars[idx].width, width);
	}

	for (const Table::Row& subrow : row.get_subrows())
	    calculate_widths(table, subrow, false, indent + 1);
    }


    /**
     * Including global_indent and grid. Excluding screen_width.
     */
    size_t
    Table::OutputInfo::calculate_total_width(const Table& table) const
    {
	size_t total_width = table.global_indent;

	bool first = true;

	for (size_t idx = 0; idx < column_vars.size(); ++idx)
	{
	    if (column_vars[idx].hidden)
		continue;

	    if (first)
		first = false;
	    else
		total_width += 2 + (table.show_grid ? 1 : 0);

	    total_width += column_vars[idx].width;
	}

	return total_width;
    }


    /**
     * So far only one column can be abbreviated.
     */
    void
    Table::OutputInfo::calculate_abbriviated_widths(const Table& table)
    {
	size_t total_width = calculate_total_width(table);

	if (total_width <= table.screen_width)
	    return;

	size_t too_much = total_width - table.screen_width;

	for (size_t idx = 0; idx < table.column_params.size(); ++idx)
	{
	    if (table.column_params[idx].abbreviate)
	    {
		column_vars[idx].width = max(column_vars[idx].width - too_much, (size_t) 5);
		break;
	    }
	}
    }


    void
    Table::output(std::ostream& s, const OutputInfo& output_info, const Table::Row& row, bool is_header,
		  const vector<bool>& lasts) const
    {
	s << string(global_indent, ' ');

	const vector<string>& columns = row.get_columns();

	for (size_t idx = 0; idx < output_info.column_vars.size(); ++idx)
	{
	    if (output_info.column_vars[idx].hidden)
		continue;

	    string column = idx < columns.size() ? columns[idx] : "";

	    if (!is_header && column_params[idx].trim)
		column = output_info.trimmed(column, column_params[idx].align, output_info.column_vars[idx].trim);

	    bool first = idx == 0;
	    bool last = idx == output_info.column_vars.size() - 1;

	    size_t extra = (idx == tree_idx) ? 2 * lasts.size() : 0;

	    if (last && column.empty())
		break;

	    if (!first)
		s << " ";

	    if (idx == tree_idx)
	    {
		for (size_t tl = 0; tl < lasts.size(); ++tl)
		{
		    if (tl == lasts.size() - 1)
			s << (lasts[tl] ? glyph(4) : glyph(3));
		    else
			s << (lasts[tl] ? glyph(6) : glyph(5));
		}
	    }

	    size_t width = mbs_width(column);

	    if (column_params[idx].align == Align::RIGHT)
	    {
		if (width < output_info.column_vars[idx].width - extra)
		    s << string(output_info.column_vars[idx].width - width - extra, ' ');
	    }

	    if (width > output_info.column_vars[idx].width - extra)
	    {
		const char* ellipsis = glyph(7);
		s << mbs_substr_by_width(column, 0, output_info.column_vars[idx].width - extra - mbs_width(ellipsis))
		  << ellipsis;
	    }
	    else
		s << column;

	    if (last)
		break;

	    if (column_params[idx].align == Align::LEFT)
	    {
		if (width < output_info.column_vars[idx].width - extra)
		    s << string(output_info.column_vars[idx].width - width - extra, ' ');
	    }

	    s << " ";

	    if (show_grid)
		s << glyph(0);
	}

	s << '\n';

	const vector<Table::Row>& subrows = row.get_subrows();
	for (size_t i = 0; i < subrows.size(); ++i)
	{
	    vector<bool> sub_lasts = lasts;
	    sub_lasts.push_back(i == subrows.size() - 1);
	    output(s, output_info, subrows[i], false, sub_lasts);
	}
    }


    /**
     * Output grid line under header.
     */
    void
    Table::output(std::ostream& s, const OutputInfo& output_info) const
    {
	s << string(global_indent, ' ');

	for (size_t idx = 0; idx < output_info.column_vars.size(); ++idx)
	{
	    if (output_info.column_vars[idx].hidden)
		continue;

	    for (size_t j = 0; j < output_info.column_vars[idx].width; ++j)
		s << glyph(1);

	    if (idx == output_info.column_vars.size() - 1)
		break;

	    s << glyph(1) << glyph(2) << glyph(1);
	}

	s << '\n';
    }


    bool
    Table::has_id(Id id) const
    {
	for (size_t idx = 0; idx < column_params.size(); ++idx)
	    if (column_params[idx].id == id)
		return true;

	return false;
    }


    size_t
    Table::id_to_idx(Id id) const
    {
	for (size_t idx = 0; idx < column_params.size(); ++idx)
	    if (column_params[idx].id == id)
		return idx;

	throw runtime_error("id not found");
    }


    string&
    Table::Row::operator[](Id id)
    {
	size_t idx = table.id_to_idx(id);

	if (columns.size() < idx + 1)
	    columns.resize(idx + 1);

	return columns[idx];
    }


    Style
    Table::auto_style()
    {
	return strcmp(nl_langinfo(CODESET), "UTF-8") == 0 ? Style::LIGHT : Style::ASCII;
    }


    Table::Table()
	: header(*this)
    {
	screen_width = get_screen_width();
    }


    Table::Table(std::initializer_list<Cell> init)
	: Table()
    {
	for (const Cell& cell : init)
	{
	    header.add(cell.name);
	    column_params.emplace_back(cell.id, cell.align);
	}
    }


    Table::Table(const vector<Cell>& init)
	: Table()
    {
	for (const Cell& cell : init)
	{
	    header.add(cell.name);
	    column_params.emplace_back(cell.id, cell.align);
	}
    }


    void
    Table::set_min_width(Id id, size_t min_width)
    {
	size_t idx = id_to_idx(id);
	column_params[idx].min_width = min_width;
    }


    void
    Table::set_visibility(Id id, Visibility visibility)
    {
	size_t idx = id_to_idx(id);
	column_params[idx].visibility = visibility;
    }


    void
    Table::set_abbreviate(Id id, bool abbreviate)
    {
	size_t idx = id_to_idx(id);
	column_params[idx].abbreviate = abbreviate;
    }


    void
    Table::set_trim(Id id, bool trim)
    {
	size_t idx = id_to_idx(id);
	column_params[idx].trim = trim;
    }


    void
    Table::set_tree_id(Id id)
    {
	tree_idx = id_to_idx(id);
    }


    std::ostream&
    operator<<(std::ostream& s, const Table& table)
    {
	// calculate hidden and widths

	Table::OutputInfo output_info(table);

	// output header and rows

	if (table.show_header)
	    table.output(s, output_info, table.header, true, {});

	if (table.show_header && table.show_grid)
	    table.output(s, output_info);

	for (const Table::Row& row : table.rows)
	    table.output(s, output_info, row, false, {});

	return s;
    }


    const char*
    Table::glyph(unsigned int i) const
    {
	const char* glyphs[][8] = {
	    { "|", "-", "+", "+-", "+-", "| ", "  ", "..." },	// ASCII
	    { "│", "─", "┼", "├─", "└─", "│ ", "  ", "…" },	// LIGHT
	    { "┃", "━", "╋", "├─", "└─", "│ ", "  ", "…" },	// HEAVY
	    { "║", "═", "╬", "├─", "└─", "│ ", "  ", "…" },	// DOUBLE
	};

	return glyphs[(unsigned int)(style)][i];
    }

}
