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

#ifndef LENSOR_OS_CPU_H
#define LENSOR_OS_CPU_H

#include <format>

#include <cstr.h>
#include <integers.h>
#include <linked_list.h>
#include <memory.h>
#include <uart.h>

/* System
 * |- Central Processing Unit(s)
 * |  `- CPUDescription
 * |     |- Capabilities (feature set)
 * |     |- Features Enabled
 * |     `- CPUs present
 * |
 * |- Random Access Memory
 * |  |- PageFrameAllocator (gAlloc)
 * |  |  `- Bitmap of used/free pages
 * |  `- PageTableManager (gPTM)
 * |     `- Modification operations for a PageTable (map_memory, unmap_memory)
 * |
 * |- Input/Output Bus
 * |  |- Programmable Interrupt Controller (PIC)
 * |  |- Programmable Interval Timer (PIT)
 * |  `- Real Time Clock (RTC)
 * |
 * |- ACPI
 * |  `- High Precision Event Timer (HPET)
 * |
 * `- PCI(e) Bus
 *
 */

class CPUDescription;
class CPU {
    friend class CPUDescription;
public:
    CPU () {}

    explicit CPU (CPUDescription* description)
        : Description(description) {}

    CPU (CPUDescription* description
         , u16 logCore
         , u16 core
         , u16 numaChip
         , u16 numaDomain
         , u16 apicID)
        : Description(description)
        , LogicalCore(logCore)
        , Core(core)
        , NUMAChip(numaChip)
        , NUMADomain(numaDomain)
        , APICID(apicID) {}

private:
    CPUDescription* Description { nullptr };
    u16 LogicalCore { 0 };
    u16 Core        { 0 };
    // See non-uniform memory access resources above.
    u16 NUMAChip    { 0 };
    u16 NUMADomain  { 0 };
    // Advanced Programmable Interrupt Controller Identifier.
    // Different for each CPU; found within ACPI MADT table.
    u16 APICID      { 0 };

    /* Print CPU data in format of:
     * CPU <NUMA domain>:<NUMA chip>:<Physical core>:<Logical core>
     */
    void print_debug() {
        std::print("  CPU {}:{}:{}:{}\n",
            NUMADomain, NUMAChip, Core, LogicalCore);
    }
};

/* CPUDescription holds the data common to all CPUs present on the system
 *   This includes things like Vendor, Brand, Cache Sizes, Feature Flags, etc.
 */
class CPUDescription {
public:
    CPUDescription() {}

    // NOTE: This is not null-terminated! Do not print this directly!
    // VendorID has a size of twelve bytes, so ensure to not read outside that.
    char* get_vendor_id() {
        return VendorID;
    }

    void set_vendor_id(char id[12]) {
        memcpy(&VendorID[0], &id[0], 12 * sizeof(char));
    }

    void add_cpu(const CPU& cpu) { CPUs.add(cpu); }

    void set_logical_core_bits (u8 bits) { LogicalCoreBits  = bits; }
    void set_physical_core_bits(u8 bits) { PhysicalCoreBits = bits; }

    void print_debug() {
        std::print("[CPU]: Description Dump:\n"
                   "  Capabilites:\n"
                   "    CPUID: {}\n"
                   "    FXSR:  {}\n"
                   "    FPU:   {}\n"
                   "    SSE:   {}\n"
                   "    XSAVE: {}\n"
                   "    AVX:   {}\n"
                   "  Enabled:\n"
                   "    FXSR:  {}\n"
                   "    FPU:   {}\n"
                   "    SSE:   {}\n"
                   "    XSAVE: {}\n"
                   "    AVX:   {}\n\n"
                   , CPUIDCapable
                   , FXSRCapable
                   , FPUCapable
                   , SSECapable
                   , XSAVECapable
                   , AVXCapable
                   , FXSREnabled
                   , FPUEnabled
                   , SSEEnabled
                   , XSAVEEnabled
                   , AVXEnabled);

        CPUs.for_each([](auto* it){ it->value().print_debug(); });
        std::print("\n");
    }

    // Feature flag setters
    void set_cpuid_capable() { CPUIDCapable = true; }
    void set_fxsr_capable()  { FXSRCapable = true;  }
    void set_fxsr_enabled()  { FXSREnabled = true;  }
    void set_fpu_capable()   { FPUCapable = true;   }
    void set_fpu_enabled()   { FPUEnabled = true;   }
    void set_sse_capable()   { SSECapable = true;   }
    void set_sse_enabled()   { SSEEnabled = true;   }
    void set_xsave_capable() { XSAVECapable = true; }
    void set_xsave_enabled() { XSAVEEnabled = true; }
    void set_avx_capable()   { AVXCapable = true;   }
    void set_avx_enabled()   { AVXEnabled = true;   }
    // Feature flag getters
    bool cpuid_capable() { return CPUIDCapable; }
    bool fxsr_capable()  { return FXSRCapable;  }
    bool fxsr_enabled()  { return FXSREnabled;  }
    bool fpu_capable()   { return FPUCapable;   }
    bool fpu_enabled()   { return FPUEnabled;   }
    bool sse_capable()   { return SSECapable;   }
    bool sse_enabled()   { return SSEEnabled;   }
    bool xsave_capable() { return XSAVECapable; }
    bool xsave_enabled() { return XSAVEEnabled; }
    bool avx_capable()   { return AVXCapable;   }
    bool avx_enabled()   { return AVXEnabled;   }

private:
    // Used for CPU Logical/Physical core number calculation from APIC ID.
    u8 LogicalCoreBits { 0 };
    u8 PhysicalCoreBits { 0 };
    /* CPU Feature Flags
     *   Once enabled, can not be disabled.
     */
    bool CPUIDCapable { false };
    bool FXSRCapable  { false };
    bool FXSREnabled  { false };
    bool FPUCapable   { false };
    bool FPUEnabled   { false };
    bool SSECapable   { false };
    bool SSEEnabled   { false };
    bool XSAVECapable { false };
    bool XSAVEEnabled { false };
    bool AVXCapable   { false };
    bool AVXEnabled   { false };
    // 12-character string that represents the CPU vendor
    char VendorID[12] { ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
    // List of central processing units (why call them central anymore??)
    SinglyLinkedList<CPU> CPUs;
};

#endif /* LENSOR_OS_CPU_H */
