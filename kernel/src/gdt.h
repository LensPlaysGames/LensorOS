#ifndef LENSOR_OS_GDT_H
#define LENSOR_OS_GDT_H

#include "integers.h"

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
    /// Limit 15:0
    u16 Limit0;
    /// Base 15:0
    u16 Base0;
    /// Base 23:16
    u8 Base1;
    /// 0b00000000
    ///          =  Accessed
    ///         =   Readable/Writable
    ///        =    Direction/Conforming
    ///       =     Executable
    ///      =      Descriptor Type
    ///    ==       Descriptor Privilege Level
    ///   =         Segment Present
    u8 AccessByte;
    /// 0b00000000
    ///       ==== Limit 19:16
    ///      =     Available
    ///     =      64-bit code segment
    ///    =       Default Operation Size
    ///   =        Granularity
    u8 Limit1_Flags;
    /// Base 31:24
    u8 Base2;

    // Limit is 20 bits and spread across two variables.
    // This helper function makes it easy to set the limit.
    void set_limit(u32 limit) {
        u8 flags = Limit1_Flags;
        Limit0 = limit;
        Limit1_Flags = limit >> 16;
        Limit1_Flags |= flags;
    }

    // Base is 32 bits and spread across three variables.
    // This helper function makes it easy to set the base address.
    void set_base(u32 base) {
        Base0 = base;
        Base1 = base >> 16;
        Base2 = base >> 24;
    }
} __attribute__((packed));

struct TSS_GDTEntry : public GDTEntry {
    /// Base 63:32
    u32 Base3;
    u32 Reserved { 0 };

    TSS_GDTEntry(GDTEntry entry, u32 b3)
        : GDTEntry(entry)
      , Base3(b3) {}
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
    GDTEntry Ring3Null; // 0x18
    GDTEntry Ring3Code; // 0x20
    GDTEntry Ring3Data; // 0x28
    TSS_GDTEntry TSS;   // 0x30
}__attribute__((packed)) __attribute__((aligned(0x1000)));

extern GDT gGDT;
extern GDTDescriptor gGDTD;

extern "C" void LoadGDT(GDTDescriptor* gdtDescriptor);

#endif
