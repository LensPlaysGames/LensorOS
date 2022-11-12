/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#ifndef LENSOR_OS_TIME_H
#define LENSOR_OS_TIME_H

#include <rtc.h>
#include <format>

namespace Time {
    struct tm {
        int seconds;                  // seconds,  0--59
        int minutes;                  // minutes,  0--59
        int hours;                    // hours, 0 to 23
        int day_of_month;             // day of the month, 1--31
        int month;                    // month, 0--11
        int years_since_1900;         // The number of years since 1900
        int day_of_week;              // day of the week, 0--6
        int day_of_year;              // day in the year, 0--365
        int is_daylight_savings_time; // daylight saving time
    };

    enum Months {
        January,
        February,
        March,
        April,
        May,
        June,
        July,
        August,
        September,
        October,
        November,
        December,

        JAN = January,
        FEB = February,
        MAR = March,
        APR = April,
        MAY = May,
        JUN = June,
        JUL = July,
        AUG = August,
        SEP = September,
        OCT = October,
        NOV = November,
        DEC = December,
    };

    // These assume no leap year!
    inline constexpr u8 month_lengths[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    inline constexpr u16 days_into_year_by_month[12] = {
        0,
        month_lengths[JAN],
        month_lengths[JAN] + month_lengths[FEB],
        month_lengths[JAN] + month_lengths[FEB] + month_lengths[MAR],
        month_lengths[JAN] + month_lengths[FEB] + month_lengths[MAR] + month_lengths[APR],
        month_lengths[JAN] + month_lengths[FEB] + month_lengths[MAR] + month_lengths[APR] + month_lengths[MAY],
        month_lengths[JAN] + month_lengths[FEB] + month_lengths[MAR] + month_lengths[APR] + month_lengths[MAY] + month_lengths[JUN],
        month_lengths[JAN] + month_lengths[FEB] + month_lengths[MAR] + month_lengths[APR] + month_lengths[MAY] + month_lengths[JUN] + month_lengths[JUL],
        month_lengths[JAN] + month_lengths[FEB] + month_lengths[MAR] + month_lengths[APR] + month_lengths[MAY] + month_lengths[JUN] + month_lengths[JUL] + month_lengths[AUG],
        month_lengths[JAN] + month_lengths[FEB] + month_lengths[MAR] + month_lengths[APR] + month_lengths[MAY] + month_lengths[JUN] + month_lengths[JUL] + month_lengths[AUG] + month_lengths[SEP],
        month_lengths[JAN] + month_lengths[FEB] + month_lengths[MAR] + month_lengths[APR] + month_lengths[MAY] + month_lengths[JUN] + month_lengths[JUL] + month_lengths[AUG] + month_lengths[SEP] + month_lengths[OCT],
        month_lengths[JAN] + month_lengths[FEB] + month_lengths[MAR] + month_lengths[APR] + month_lengths[MAY] + month_lengths[JUN] + month_lengths[JUL] + month_lengths[AUG] + month_lengths[SEP] + month_lengths[OCT] + month_lengths[NOV],
    };


    void fill_tm(tm* time) {
        time->seconds      = gRTC.Time.second;
        time->minutes      = gRTC.Time.minute;
        time->hours        = gRTC.Time.hour;
        time->day_of_week  = gRTC.Time.weekday;
        time->day_of_month = gRTC.Time.date;
        time->month        = gRTC.Time.month;

        // TODO: Have some way to tell if we are in leap year or not, and add one.
        time->day_of_year = days_into_year_by_month[gRTC.Time.month] + gRTC.Time.date;

        // This will become inaccurate in the year 2100...
        time->years_since_1900 = 100 + gRTC.Time.year;

        // Figure it out for yourself, you filthy animal.
        time->is_daylight_savings_time = -1;

    }
}

namespace std {
template <>
struct formatter<Time::tm> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Time::tm& t, FormatContext& ctx) {
        return format_to(ctx.out(), "{:02d}:{:02d}:{:02d} {:02d}/{:02d}/{:04d}",
            t.hours, t.minutes, t.seconds,
            t.day_of_month, t.month, t.years_since_1900);
    }
};
}

#endif /* LENSOR_OS_TIME_H */
