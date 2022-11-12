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

#include <gpt.h>

#include <ahci.h>
#include <debug.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_GPT

#ifdef DEBUG_GPT
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...)
#endif

namespace GPT {
    constexpr const char* HEADER_SIGNATURE = "EFI PART";
    bool is_gpt_present(StorageDeviceDriver* driver) {
        if (driver == nullptr) {
            std::print("[GPT]: GPT can not be present on driver that is nullptr!\n");
            return false;
        }
        DBGMSG("[GPT]: Checking for valid GPT\n");
        Header hdr;
        driver->read_raw(512, sizeof hdr, (u8*)&hdr);
        // Validate GPT Header
        if (hdr.Revision == 0) {
            DBGMSG("  ERROR: Revision is not zero\n");
            return false;
        }
        for (u8 i = 0; i < 8; ++i) {
            if (hdr.Signature[i] != HEADER_SIGNATURE[i]) {
                DBGMSG("  ERROR: Signature doesn't match\n");
                return false;
            }
        }
        DBGMSG("  Valid GPT\n");
        return true;
    }
}
