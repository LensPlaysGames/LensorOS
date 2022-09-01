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
#ifndef LENSOR_OS_GDT_H
#define LENSOR_OS_GDT_H

#include <integers.h>

struct GDTDescriptor {
    u16 Size;
    u64 Offset;

    GDTDescriptor() {}
    GDTDescriptor(u16 size, u64 offset)
        : Size(size), Offset(offset) {}
} __attribute__((packed));

/// Info taken from "Section 3.4.5: Segment Descriptors" and
///   "Figure 3-8: Segment Descriptor" of the Intel Software Developer Manual, Volume 3-A.
/// NOTE: Each type of entry has a different layout, below only shows the generic.
///       See "Section 5.2: Fields and Flags Used for Segment-Level and Page-Level Protection" and
///         "Figure 5-1: Descriptor Fields Used for Protection" of the Intel Software Manual, Volume 3-A.
struct GDTEntry {
public:
    GDTEntry() {}
    GDTEntry(u32 base, u32 limit, u8 access, u8 flags) {
        set_base(base);
        set_limit(limit);
        set_access(access);
        set_flags(flags);
    }

    u32 base() {
        return Base0
            | Base1 << 16
            | Base2 << 24;
    }

    u32 limit() {
        return Limit0
            | (Limit1_Flags & 0xf) << 16;
    }

    u8 access() {
        return AccessByte;
    }

    u8 flags() {
        return Limit1_Flags & 0xf0;
    }

    // Limit is 20 bits and spread across two variables.
    // This helper function makes it easy to set the limit.
    void set_limit(u32 limit) {
        u8 flags = Limit1_Flags & 0xf0;
        Limit0 = limit;
        Limit1_Flags = (limit >> 16) & 0x0f;
        Limit1_Flags |= flags;
    }

    // Base is 32 bits and spread across three variables.
    // This helper function makes it easy to set the base address.
    void set_base(u32 base) {
        Base0 = base;
        Base1 = base >> 16;
        Base2 = base >> 24;
    }

    void set_access(u8 access) {
        AccessByte = access;
    }

    void set_flags(u8 flags) {
        u8 limitNibble = Limit1_Flags & 0x0f;
        Limit1_Flags = flags & 0xf0;
        Limit1_Flags |= limitNibble;
    }

private:
    /// Limit 15:0
    u16 Limit0 { 0 };
    /// Base 15:0
    u16 Base0 { 0 };
    /// Base 23:16
    u8 Base1 { 0 };
    /// 0b00000000
    ///          =  Accessed
    ///         =   Readable/Writable
    ///        =    Direction/Conforming
    ///       =     Executable
    ///      =      Descriptor Type
    ///    ==       Descriptor Privilege Level
    ///   =         Segment Present
    u8 AccessByte { 0 };
    /// 0b00000000
    ///       ==== Limit 19:16
    ///      =     Available
    ///     =      64-bit segment
    ///    =       Default Operation Size
    ///   =        Granularity (set means limit in 4kib pages)
    u8 Limit1_Flags { 0 };
    /// Base 31:24
    u8 Base2 { 0 };
} __attribute__((packed));

class TSS_GDTEntry : public GDTEntry {
public:
    TSS_GDTEntry() {}
    TSS_GDTEntry(GDTEntry entry)
        : GDTEntry(entry)
    {
        Reserved = 0;
    }

    u64 base() {
        return GDTEntry::base()
            | (u64)Base3 << 32;
    }

    void set_base(u64 base) {
        GDTEntry::set_base(base);
        Base3 = base >> 32;
    }

private:
    /// Base 63:32
    u32 Base3 { 0 };
    u32 Reserved { 0 };
} __attribute__((packed));

/// Global Descriptor Table
/// The `Code` entry is loaded into CS,
///   the `Data` entry is loaded into all other segment selector registers.
/// `Ring` refers to the common thought-process in computing of 'protection rings'
/// Picture concentric circles (like a dartboard, or a target).
/// Starting at the center, count up from 0 until there are no more rings.
/// A given ring may access anything in any ring larger than itself;
///   however, a ring may not access anything in any ring smaller than itself.
/// This allows for the kernel (ring zero) to access all programs, drivers, etc.
///   but dis-allow userland programs from tampering with the kernel, drivers, etc.
struct GDT {
    GDTEntry Null;      // 0x00
    GDTEntry Ring0Code; // 0x08
    GDTEntry Ring0Data; // 0x10
    GDTEntry Ring3Code; // 0x18
    GDTEntry Ring3Data; // 0x20
    TSS_GDTEntry TSS;   // 0x28, 0x30
} __attribute__((aligned(0x1000)));

void setup_gdt();

extern GDT gGDT;
extern GDTDescriptor gGDTD;

extern "C" void LoadGDT(GDTDescriptor* gdtDescriptor);

#endif
