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
#include <memory/common.h>

namespace ACPI {
struct MCFGHeader;
}

namespace PCI {
enum class BarType {
    Memory,
    IO,
};

constexpr inline bool bar_is64(u32 BAR0) {
    return (BAR0 & 0b110) == 0b100;
}

constexpr inline u64 get_bar_address(u32 BAR0, u32 BAR1) {
    u64 BAR = BAR0;
    // clear bottom four bits
    BAR &= ~u64(0xf);
    if (bar_is64(BAR0))
        BAR |= u64(BAR1) << 32;
    BAR = Memory::FROM_FRAME_POINTER(BAR);
    return BAR;
}

constexpr inline BarType get_bar_type(u32 BAR) {
    return BAR & 1
               ? BarType::IO
               : BarType::Memory;
}

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

u64 get_bar_size(PCIHeader0* PCIHeader);
constexpr inline u64 get_bar_address(const PCIHeader0* PCIHeader) {
    return get_bar_address(PCIHeader->BAR0, PCIHeader->BAR1);
}

void enumerate_pci(ACPI::MCFGHeader* mcfg);

extern const char* DeviceClasses[];
const char* get_vendor_name(u16 vendorID);
const char* get_device_name(u16 vendorID, u16 deviceID);
const char* get_subclass_name(u8 classCode, u8 subclassCode);
const char* get_prog_if_name(u8 _class, u8 subclass, u8 progIF);
}  // namespace PCI
#endif
