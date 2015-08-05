/*
 * Copyright (c) [2015] Red Hat, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef SNAPPER_REF_COMPARISON_H
#define SNAPPER_REF_COMPARISON_H

#include "RefCounter.h"
#include "snapper/Comparison.h"

struct XComparison : public RefCounter
{
    XComparison(snapper::Comparison* cmp) : RefCounter(), cmp(cmp) {}
    ~XComparison() { assert(use_count() == 0); delete cmp; }
    const snapper::Comparison* cmp;
};

#endif

