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

}  // namespace Time
