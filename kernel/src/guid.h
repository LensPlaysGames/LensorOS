#ifndef LENSOR_OS_GUID_H
#define LENSOR_OS_GUID_H

#include "integers.h"
#include "memory.h"

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
