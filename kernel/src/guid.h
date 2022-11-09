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

#ifndef LENSOR_OS_GUID_H
#define LENSOR_OS_GUID_H

#include <integers.h>
#include <memory.h>
#include <format>

struct GUID {
    u32 Data1;
    u16 Data2;
    u16 Data3;
    u8 Data4[8];

    bool operator == (const GUID& rhs) const {
        return !memcmp((void*)this, (void*)&rhs, sizeof(GUID));
    }

    bool operator != (const GUID& rhs) const {
        return !(*this == rhs);
    }
};

namespace std {
template <>
struct formatter<GUID, char> {
    constexpr auto parse(basic_format_parse_context<char>& ctx) {
        if (*ctx.begin() != '}') { __detail::__invalid_format_string(); }
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const GUID& guid, FormatContext& ctx) {
        return format_to(ctx.out(), "{:08x}-{:04x}-{:04x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    }
};
}

// Debug purposes only!
#include "cstr.h"
#include "uart.h"
static inline void print_guid(const GUID& id) {
    UART::out(to_hexstring(id.Data1));
    UART::outc('-');
    UART::out(to_hexstring(id.Data2));
    UART::outc('-');
    UART::out(to_hexstring(id.Data3));
    UART::outc('-');
    u64 data4 { 0 };
    for (int i = 0, j = 1; i < 2; ++i, --j)
        data4 |= id.Data4[i] << (j * 8);

    UART::out(to_hexstring(data4));
    UART::outc('-');

    data4 = 0;
    for (int i = 2, j = 5; i < 8; ++i, --j)
        data4 |= (u64)id.Data4[i] << (j * 8);

    UART::out(to_hexstring(data4));
}

#endif /* LENSOR_OS_GUID_H */
