/*
 * Copyright (c) [2011-2014] Novell, Inc.
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


namespace snapper
{

bool
equal_year(const struct tm& tmp1, const struct tm& tmp2);

bool
equal_quarter(const struct tm& tmp1, const struct tm& tmp2);

bool
equal_month(const struct tm& tmp1, const struct tm& tmp2);

bool
equal_week(const struct tm& tmp1, const struct tm& tmp2);

bool
equal_day(const struct tm& tmp1, const struct tm& tmp2);

bool
equal_hour(const struct tm& tmp1, const struct tm& tmp2);

}
