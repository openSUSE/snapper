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
	void calculate_widths(const Table& table, const Table::Row& row, unsigned indent);
	size_t calculate_total_width(const Table& table) const;
	void calculate_abbriviated_widths(const Table& table);

	vector<bool> hidden;
	vector<size_t> widths;
    };


    Table::OutputInfo::OutputInfo(const Table& table)
    {
	// calculate hidden, default to false

	hidden.resize(table.header.get_columns().size());

	for (size_t i = 0; i < table.visibilities.size(); ++i)
	{
	    if (table.visibilities[i] == Visibility::AUTO || table.visibilities[i] == Visibility::OFF)
		hidden[i] = true;
	}

	for (const Table::Row& row : table.rows)
	    calculate_hidden(table, row);

	// calculate widths

	widths = table.min_widths;

	if (table.show_header)
	    calculate_widths(table, table.header, 0);

	for (const Table::Row& row : table.rows)
	    calculate_widths(table, row, 0);

	calculate_abbriviated_widths(table);
    }


    void
    Table::OutputInfo::calculate_hidden(const Table& table, const Table::Row& row)
    {
	const vector<string>& columns = row.get_columns();

	for (size_t i = 0; i < min(columns.size(), table.visibilities.size()); ++i)
	{
	    if (table.visibilities[i] == Visibility::AUTO && !columns[i].empty())
		hidden[i] = false;
	}

	for (const Table::Row& subrow : row.get_subrows())
	    calculate_hidden(table, subrow);
    }


    void
    Table::OutputInfo::calculate_widths(const Table& table, const Table::Row& row, unsigned indent)
    {
	const vector<string>& columns = row.get_columns();

	if (columns.size() > widths.size())
	    widths.resize(columns.size());

	for (size_t i = 0; i < columns.size(); ++i)
	{
	    if (hidden[i])
		continue;

	    size_t width = mbs_width(columns[i]);

	    if (i == table.tree_index)
		width += 2 * indent;

	    widths[i] = max(widths[i], width);
	}

	for (const Table::Row& subrow : row.get_subrows())
	    calculate_widths(table, subrow, indent + 1);
    }


    /**
     * Including global_indent and grid. Excluding screen_width.
     */
    size_t
    Table::OutputInfo::calculate_total_width(const Table& table) const
    {
	size_t total_width = table.global_indent;

	bool first = true;

	for (size_t i = 0; i < widths.size(); ++i)
	{
	    if (hidden[i])
		continue;

	    if (first)
		first = false;
	    else
		total_width += 2 + (table.show_grid ? 1 : 0);

	    total_width += widths[i];
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

	for (size_t i = 0; i < table.abbreviates.size(); ++i)
	{
	    if (table.abbreviates[i])
	    {
		widths[i] = max(widths[i] - too_much, (size_t) 5);
		break;
	    }
	}
    }


    void
    Table::output(std::ostream& s, const OutputInfo& output_info, const Table::Row& row, const vector<bool>& lasts) const
    {
	s << string(global_indent, ' ');

	const vector<string>& columns = row.get_columns();

	for (size_t i = 0; i < output_info.widths.size(); ++i)
	{
	    if (output_info.hidden[i])
		continue;

	    string column = i < columns.size() ? columns[i] : "";

	    bool first = i == 0;
	    bool last = i == output_info.widths.size() - 1;

	    size_t extra = (i == tree_index) ? 2 * lasts.size() : 0;

	    if (last && column.empty())
		break;

	    if (!first)
		s << " ";

	    if (i == tree_index)
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

	    if (aligns[i] == Align::RIGHT)
	    {
		if (width < output_info.widths[i] - extra)
		    s << string(output_info.widths[i] - width - extra, ' ');
	    }

	    if (width > output_info.widths[i] - extra)
	    {
		const char* ellipsis = glyph(7);
		s << mbs_substr_by_width(column, 0, output_info.widths[i] - extra - mbs_width(ellipsis))
		  << ellipsis;
	    }
	    else
		s << column;

	    if (last)
		break;

	    if (aligns[i] == Align::LEFT)
	    {
		if (width < output_info.widths[i] - extra)
		    s << string(output_info.widths[i] - width - extra, ' ');
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
	    output(s, output_info, subrows[i], sub_lasts);
	}
    }


    /**
     * Output grid line under header.
     */
    void
    Table::output(std::ostream& s, const OutputInfo& output_info) const
    {
	s << string(global_indent, ' ');

	for (size_t i = 0; i < output_info.widths.size(); ++i)
	{
	    if (output_info.hidden[i])
		continue;

	    for (size_t j = 0; j < output_info.widths[i]; ++j)
		s << glyph(1);

	    if (i == output_info.widths.size() - 1)
		break;

	    s << glyph(1) << glyph(2) << glyph(1);
	}

	s << '\n';
    }


    bool
    Table::has_id(Id id) const
    {
	for (size_t i = 0; i < ids.size(); ++i)
	    if (ids[i] == id)
		return true;

	return false;
    }


    size_t
    Table::id_to_index(Id id) const
    {
	for (size_t i = 0; i < ids.size(); ++i)
	    if (ids[i] == id)
		return i;

	throw runtime_error("id not found");
    }


    string&
    Table::Row::operator[](Id id)
    {
	size_t i = table.id_to_index(id);

	if (columns.size() < i + 1)
	    columns.resize(i + 1);

	return columns[i];
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
	    ids.push_back(cell.id);
	    aligns.push_back(cell.align);
	}
    }


    Table::Table(const vector<Cell>& init)
	: Table()
    {
	for (const Cell& cell : init)
	{
	    header.add(cell.name);
	    ids.push_back(cell.id);
	    aligns.push_back(cell.align);
	}
    }


    void
    Table::set_min_width(Id id, size_t min_width)
    {
	size_t i = id_to_index(id);

	if (min_widths.size() < i + 1)
	    min_widths.resize(i + 1);

	min_widths[i] = min_width;
    }


    void
    Table::set_visibility(Id id, Visibility visibility)
    {
	size_t i = id_to_index(id);

	if (visibilities.size() < i + 1)
	    visibilities.resize(i + 1);

	visibilities[i] = visibility;
    }


    void
    Table::set_abbreviate(Id id, bool abbreviate)
    {
	size_t i = id_to_index(id);

	if (abbreviates.size() < i + 1)
	    abbreviates.resize(i + 1);

	abbreviates[i] = abbreviate;
    }


    void
    Table::set_tree_id(Id id)
    {
	tree_index = id_to_index(id);
    }


    std::ostream&
    operator<<(std::ostream& s, const Table& table)
    {
	// calculate hidden and widths

	Table::OutputInfo output_info(table);

	// output header and rows

	if (table.show_header)
	    table.output(s, output_info, table.header, {});

	if (table.show_header && table.show_grid)
	    table.output(s, output_info);

	for (const Table::Row& row : table.rows)
	    table.output(s, output_info, row, {});

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
