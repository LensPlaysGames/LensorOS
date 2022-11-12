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

#include <ahci.h>
#include <format>
#include <integers.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_AHCI

#ifdef DEBUG_AHCI
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...) void()
#endif

namespace AHCI {
    const char* port_type_strings[5] = {
        "None",
        "SATA",
        "SEMB",
        "PM",
        "SATAPI"
    };
    const char* port_type_string(PortType p) {
        return port_type_strings[p];
    }

    PortType get_port_type(HBAPort* port) {
        u32 sataStatus = port->SataStatus;
        u8 interfacePowerManagement = (sataStatus >> 8) & 0b111;
        u8 deviceDetection = sataStatus & 0b111;
        if (deviceDetection != HBA_PORT_DEVICE_PRESENT
            || interfacePowerManagement != HBA_PORT_IPM_ACTIVE)
        {
            // Device is not present or active.
            return PortType::None;
        }
        switch (port->signature) {
        case SATA_SIG_ATAPI:
            return PortType::SATAPI;
        case SATA_SIG_ATA:
            return PortType::SATA;
        case SATA_SIG_SEMB:
            return PortType::SEMB;
        case SATA_SIG_PM:
            return PortType::PM;
        default:
            return PortType::None;
        }
    }

}
