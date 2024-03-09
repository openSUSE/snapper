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


#include <time.h>

#include "equal-date.h"

#define isleapyear(year) \
    ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

int
yday_of_weeks_monday(const struct tm& tmp)
{
    return tmp.tm_yday - (tmp.tm_wday != 0 ? tmp.tm_wday : 7);
}


int
days_in_year(const struct tm& tmp)
{
    return isleapyear(tmp.tm_year) ? 366 : 365;
}


bool
equal_year(const struct tm& tmp1, const struct tm& tmp2)
{
    return tmp1.tm_year == tmp2.tm_year;
}


bool
equal_quarter(const struct tm& tmp1, const struct tm& tmp2)
{
    return equal_year(tmp1, tmp2) && (tmp1.tm_mon / 3) == (tmp2.tm_mon / 3);
}


bool
equal_month(const struct tm& tmp1, const struct tm& tmp2)
{
    return equal_year(tmp1, tmp2) && tmp1.tm_mon == tmp2.tm_mon;
}


bool
equal_week(const struct tm& tmp1, const struct tm& tmp2)
{
    if (tmp1.tm_year == tmp2.tm_year)
        return yday_of_weeks_monday(tmp1) == yday_of_weeks_monday(tmp2);

    if (tmp1.tm_year + 1 == tmp2.tm_year)
        return yday_of_weeks_monday(tmp1) == yday_of_weeks_monday(tmp2) + days_in_year(tmp1);

    if (tmp1.tm_year == tmp2.tm_year + 1)
        return yday_of_weeks_monday(tmp1) + days_in_year(tmp2) == yday_of_weeks_monday(tmp2);

    return false;
}


bool
equal_day(const struct tm& tmp1, const struct tm& tmp2)
{
    return equal_month(tmp1, tmp2) && tmp1.tm_mday == tmp2.tm_mday;
}


bool
equal_hour(const struct tm& tmp1, const struct tm& tmp2)
{
    return equal_day(tmp1, tmp2) && tmp1.tm_hour == tmp2.tm_hour;
}
