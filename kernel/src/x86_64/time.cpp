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

#include <time.h>
#include <x86_64/lapic.h>
#include <x86_64/pit.h>
#include <x86_64/rtc.h>

#define CACHE_LINE_SIZE 64

namespace Time {

namespace detail {
// alignas(CACHE_LINE_SIZE) to prevent cache-thrashing, where updating
// Ticks invalidates Frequency.
alignas(CACHE_LINE_SIZE) volatile size_t Ticks{0};
alignas(CACHE_LINE_SIZE) size_t Frequency{1};
size_t ApproxNanosecondsPer{nanoseconds_per_second};
}  // namespace detail

size_t current_tick() {
    return detail::Ticks;
}
size_t nanoseconds_per_tick() {
    return detail::ApproxNanosecondsPer;
}
extern "C" void do_tick() {
    detail::Ticks += 1;
}
void set_frequency(size_t hertz) {
    if (hertz == 0) return;
    detail::Frequency = hertz;
    detail::ApproxNanosecondsPer = nanoseconds_per_second / hertz;
}

size_t seconds_since_boot() {
    return detail::Ticks / detail::Frequency;
}
size_t milliseconds_since_boot() {
    return detail::Ticks * 1000 / detail::Frequency;
}

void blocking_wait_ms(size_t millisecond_amount) {
    gPIT.wait_polling(millisecond_amount);
}

void fill_tm(tm* time) {
    time->seconds = gRTC.Time.second;
    time->minutes = gRTC.Time.minute;
    time->hours = gRTC.Time.hour;
    time->day_of_week = gRTC.Time.weekday;
    time->day_of_month = gRTC.Time.date;
    time->month = gRTC.Time.month - 1;

    // TODO: Have some way to tell if we are in leap year or not, and add one.
    time->day_of_year = days_into_year_by_month[gRTC.Time.month] + gRTC.Time.date;

    time->years_since_1900 = gRTC.Time.year - 1900;

    // Figure it out for yourself, you filthy animal.
    time->is_daylight_savings_time = -1;

    // Verify Values
    time->seconds = std::min(time->seconds, 59);
    time->minutes = std::min(time->minutes, 59);
    time->hours = std::min(time->hours, 23);
    time->month = std::min(time->month, 11);
    time->day_of_week = std::min(time->day_of_week, 6);
    time->day_of_month = std::clamp(time->day_of_month, 1, 31);
    time->day_of_year = std::min(time->day_of_year, 365);
}

uint64_t mktime(const tm* t) {
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

void gmtime(usz unix_time, tm* const result) {
    if (not result) return;

    // Calculate time of day (Seconds, Minutes, Hours)
    uint32_t seconds_in_day = (uint32_t)(unix_time % 86400);
    result->seconds = seconds_in_day % 60;
    result->minutes = (seconds_in_day % 3600) / 60;
    result->hours = seconds_in_day / 3600;

    // Compute Weekday (Unix epoch 1970-01-01 was a Thursday = 4)
    int64_t days_since_epoch = (int64_t)(unix_time / 86400);
    result->day_of_week = (int)((days_since_epoch + 4) % 7);
    if (result->day_of_week < 0)
        result->day_of_week += 7;

    // Shift epoch from 1970-01-01 to a dummy year ending in a leap cycle (March 1, 0000)
    // This simplifies the leap year math significantly.
    int64_t days = days_since_epoch + 719468;

    // Era calculation (400-year cycles)
    int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    uint64_t day_of_era = (uint64_t)(days - era * 146097);  // Day of era [0, 146096]

    // Year of era [0, 399]
    uint64_t year_of_era = (day_of_era
                            - day_of_era / 1460
                            + day_of_era / 36524
                            - day_of_era / 146096)
                           / 365;
    int64_t year = ((int64_t)year_of_era) + era * 400;

    // Day of year relative to March 1st [0, 365]
    uint64_t day_of_year = day_of_era
                           - (365 * year_of_era
                              + year_of_era / 4
                              - year_of_era / 100);

    // Month relative to March 1st [0, 11]
    uint64_t month_shifted = (5 * day_of_year + 2) / 153;

    // Convert back to standard calendar values
    uint64_t day = day_of_year - (153 * month_shifted + 2) / 5 + 1;
    uint64_t month = month_shifted < 10 ? month_shifted + 3 : month_shifted - 9;

    // Adjust year if it fell in Jan/Feb of the next absolute year
    if (month <= 2) ++year;

    result->day_of_month = (int)day;
    result->month = (int)month;                   // 0-indexed
    result->years_since_1900 = (int)year - 1900;  // Years since 1900
    result->is_daylight_savings_time = false;     // UTC does not use DST

    // Calculate day of the year
    bool is_leap = (year % 4 == 0 and (year % 100 != 0 or year % 400 == 0));
    result->day_of_year = days_into_year_by_month[result->month]
                          + result->day_of_month - 1;
    if (is_leap and result->month >= 2) ++result->day_of_year;
}

}  // namespace Time
