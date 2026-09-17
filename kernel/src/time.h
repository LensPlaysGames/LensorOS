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
#include <stdint.h>

#include <format>

namespace Time {
extern size_t unix_boot_time;

constexpr size_t seconds_per_minute = 60;
constexpr size_t minutes_per_hour = 60;
constexpr size_t hours_per_day = 24;
constexpr size_t days_per_week = 7;
constexpr size_t milliseconds_per_second = 1000;
constexpr size_t microseconds_per_second = 1000000;
constexpr size_t nanoseconds_per_second = 1000000000;

constexpr size_t seconds_per_hour = seconds_per_minute * minutes_per_hour;
constexpr size_t seconds_per_day = seconds_per_hour * hours_per_day;
constexpr size_t seconds_per_week = seconds_per_day * days_per_week;
constexpr size_t microseconds_per_millisecond = microseconds_per_second / milliseconds_per_second;
constexpr size_t nanoseconds_per_millisecond = nanoseconds_per_second / milliseconds_per_second;
constexpr size_t nanoseconds_per_microsecond = nanoseconds_per_second / microseconds_per_second;

constexpr inline size_t seconds_to_milliseconds(size_t seconds) {
    return seconds * milliseconds_per_second;
}
constexpr inline size_t seconds_to_microseconds(size_t seconds) {
    return seconds * microseconds_per_second;
}
constexpr inline size_t seconds_to_nanoseconds(size_t seconds) {
    return seconds * nanoseconds_per_second;
}

constexpr inline size_t milliseconds_to_seconds(size_t milliseconds) {
    return milliseconds / milliseconds_per_second;
}
constexpr inline size_t milliseconds_to_microseconds(size_t milliseconds) {
    return milliseconds * microseconds_per_millisecond;
}
constexpr inline size_t milliseconds_to_nanoseconds(size_t milliseconds) {
    return milliseconds * nanoseconds_per_millisecond;
}

constexpr inline size_t microseconds_to_seconds(size_t microseconds) {
    return microseconds / microseconds_per_second;
}
constexpr inline size_t microseconds_to_milliseconds(size_t microseconds) {
    return microseconds / microseconds_per_millisecond;
}
constexpr inline size_t microseconds_to_nanoseconds(size_t microseconds) {
    return microseconds * nanoseconds_per_microsecond;
}

constexpr inline size_t nanoseconds_to_seconds(size_t nanoseconds) {
    return nanoseconds / nanoseconds_per_second;
}
constexpr inline size_t nanoseconds_to_milliseconds(size_t nanoseconds) {
    return nanoseconds / nanoseconds_per_millisecond;
}
constexpr inline size_t nanoseconds_to_microseconds(size_t nanoseconds) {
    return nanoseconds / nanoseconds_per_microsecond;
}

constexpr inline size_t frequency_to_nanosecond_duration(size_t hertz) {
    return nanoseconds_per_second / hertz;
}

struct tm {
    int seconds;                   // seconds,  0--59
    int minutes;                   // minutes,  0--59
    int hours;                     // hours, 0 to 23
    int day_of_month;              // day of the month, 1--31
    int month;                     // month, 0--11
    int years_since_1900;          // The number of years since 1900
    int day_of_week;               // day of the week, 0--6
    int day_of_year;               // day in the year, 0--365
    int is_daylight_savings_time;  // daylight saving time
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
    31,
    28,
    31,
    30,
    31,
    30,
    31,
    31,
    30,
    31,
    30,
    31};

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

inline void fill_tm(tm* time) {
    time->seconds = gRTC.Time.second;
    time->minutes = gRTC.Time.minute;
    time->hours = gRTC.Time.hour;
    time->day_of_week = gRTC.Time.weekday;
    time->day_of_month = gRTC.Time.date;
    time->month = gRTC.Time.month;

    // TODO: Have some way to tell if we are in leap year or not, and add one.
    time->day_of_year = days_into_year_by_month[gRTC.Time.month] + gRTC.Time.date;

    // This will become inaccurate in the year 2100...
    time->years_since_1900 = 100 + gRTC.Time.year;

    // Figure it out for yourself, you filthy animal.
    time->is_daylight_savings_time = -1;
}

inline uint64_t mktime(const tm* t) {
    uint64_t year = ((uint64_t)t->years_since_1900) + 1900;  // Actual year value
    uint64_t month = t->month + 1;                           // 1-12

    // Shift the calendar so that March is month 0, and Jan/Feb fall to the
    // previous year. This moves the "leap day anomaly" to the very end of the
    // mathematical cycle.
    year -= (month <= 2);
    uint64_t era_month = (month <= 2) ? (month + 9) : (month - 3);

    // Compute total historical days since Day 0 (using the Gregorian leap
    // year rules)
    uint64_t total_days
        = (year * 365)
          + (year / 4) - (year / 100) + (year / 400)
          + ((era_month * 306 + 5) / 10)
          + (t->day_of_month - 1);

    // Subtract the days between Day 0 and the Unix Epoch (Jan 1, 1970)
    constexpr uint64_t DAYS_TO_EPOCH = 719468;
    uint64_t unix_days = total_days - DAYS_TO_EPOCH;

    // Convert days to total seconds and tack on the hours, minutes, and
    // seconds
    return (unix_days * seconds_per_day)
           + (((uint64_t)t->hours) * seconds_per_hour)
           + (((uint64_t)t->minutes) * seconds_per_minute)
           + (uint64_t)t->seconds;
}

}  // namespace Time

namespace std {
template <>
struct formatter<Time::tm> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Time::tm& t, FormatContext& ctx) {
        return format_to(
            ctx.out(),
            "{:02d}:{:02d}:{:02d} {:02d}/{:02d}/{:04d}",
            t.hours,
            t.minutes,
            t.seconds,
            t.day_of_month,
            t.month,
            t.years_since_1900);
    }
};
}  // namespace std

#endif /* LENSOR_OS_TIME_H */
