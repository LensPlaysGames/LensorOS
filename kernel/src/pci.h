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

#ifndef LENSOR_OS_PCI_H
#define LENSOR_OS_PCI_H

#include <integers.h>

namespace ACPI {
    struct MCFGHeader;
}

namespace PCI {
    struct PCIDeviceHeader {
        u16 VendorID;
        u16 DeviceID;
        u16 Command;
        u16 Status;
        u8 RevisionID;
        u8 ProgIF;
        u8 Subclass;
        u8 Class;
        u8 CacheLineSize;
        u8 LatencyTimer;
        u8 HeaderType;
        u8 BIST;
    };

    struct PCIHeader0 {
        PCIDeviceHeader Header;
        u32 BAR0;
        u32 BAR1;
        u32 BAR2;
        u32 BAR3;
        u32 BAR4;
        u32 BAR5;
        u32 CardbusCISPtr;
        u16 SubsystemVendorID;
        u16 SubsystemID;
        u32 ExpansionROMBaseAddress;
        u8 CapabilitiesPtr;
        u8 rsv0;
        u16 rsv1;
        u32 rsv2;
        u8 InterruptLine;
        u8 InterruptPin;
        u8 MinGrant;
        u8 MaxLatency;
    };
    
    void enumerate_pci(ACPI::MCFGHeader* mcfg);

    extern const char* DeviceClasses[];
    const char* get_vendor_name(u16 vendorID);
    const char* get_device_name(u16 vendorID, u16 deviceID);
    const char* get_subclass_name(u8 classCode, u8 subclassCode);
    const char* get_prog_if_name(u8 _class, u8 subclass, u8 progIF);
}
#endif
