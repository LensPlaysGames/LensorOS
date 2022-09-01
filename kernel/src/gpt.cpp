/* Copyright 2022, Contributors To LensorOS.
All rights reserved.

This file is part of LensorOS.

LensorOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LensorOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LensorOS. If not, see <https://www.gnu.org/licenses */
#include <gpt.h>

#include <ahci.h>
#include <debug.h>
#include <smart_pointer.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_GPT

namespace GPT {
    constexpr const char* HEADER_SIGNATURE = "EFI PART";
    bool is_gpt_present(StorageDeviceDriver* driver) {
        if (driver == nullptr) {
            dbgmsg("[GPT]: GPT can not be present on driver that is nullptr!\r\n");
            return false;
        }
#ifdef DEBUG_GPT
        dbgmsg("[GPT]: Checking for valid GPT\r\n");
#endif /* DEBUG_GPT */
        SmartPtr<Header> hdr = SmartPtr<Header>(new Header);
        driver->read(512, sizeof(Header), (u8*)hdr.get());
        // Validate GPT Header
        if (hdr->Revision == 0) {
#ifdef DEBUG_GPT
            dbgmsg("  ERROR: Revision is not zero\r\n");
#endif /* DEBUG_GPT */
            return false;
        }
        for (u8 i = 0; i < 8; ++i) {
            if (hdr->Signature[i] != HEADER_SIGNATURE[i]) {
#ifdef DEBUG_GPT
                dbgmsg("  ERROR: Signature doesn't match\r\n");
#endif /* DEBUG_GPT */
                return false;
            }
        }
#ifdef DEBUG_GPT
        dbgmsg("  Valid GPT\r\n");
#endif /* DEBUG_GPT */
        return true;
    }
}
