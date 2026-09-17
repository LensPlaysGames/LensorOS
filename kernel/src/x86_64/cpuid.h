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

#ifndef LENSOR_OS_CPUID_H
#define LENSOR_OS_CPUID_H

/* Many thanks to osdev wiki: https://wiki.osdev.org/CPUID */

#include <integers.h>

// Defined in `cpuid.asm`
extern "C" u64 cpuid_support();

void cpuid(u32 code, u32& a, u32& b, u32& c, u32& d);

struct CPUIDRegisters {
    u32 A;
    u32 B;
    u32 C;
    u32 D;
};

inline void cpuid(u32 code, CPUIDRegisters& regs) {
    cpuid(code, regs.A, regs.B, regs.C, regs.D);
}

char* cpuid_string(u32 code);

/* Vendor Strings
 *  When CPUID is called with RAX equal to zero, the vendor ID string
 *    is returned in EBX,EDX, and ECX (12-byte string total).
 *  String Register Layout
 *        MSB         LSB
 *  EBX = 'u' 'n' 'e' 'G'
 *  EDX = 'I' 'e' 'n' 'i'
 *  ECX = 'l' 'e' 't' 'n'
 *
 *  This string can be tested against known values to determine exact vendor.
 */
#define CPUID_VENDOR_OLDAMD        "AMDisbetter!"
#define CPUID_VENDOR_AMD           "AuthenticAMD"
#define CPUID_VENDOR_INTEL         "GenuineIntel"
#define CPUID_VENDOR_VIA           "VIA VIA VIA "
#define CPUID_VENDOR_OLDTRANSMETA  "TransmetaCPU"
#define CPUID_VENDOR_TRANSMETA     "GenuineTMx86"
#define CPUID_VENDOR_CYRIX         "CyrixInstead"
#define CPUID_VENDOR_CENTAUR       "CentaurHauls"
#define CPUID_VENDOR_NEXGEN        "NexGenDriven"
#define CPUID_VENDOR_UMC           "UMC UMC UMC "
#define CPUID_VENDOR_SIS           "SiS SiS SiS "
#define CPUID_VENDOR_NSC           "Geode by NSC"
#define CPUID_VENDOR_RISE          "RiseRiseRise"
#define CPUID_VENDOR_VORTEX        "Vortex86 SoC"
#define CPUID_VENDOR_OLDAO486      "GenuineAO486"
#define CPUID_VENDOR_AO486         "MiSTer AO486"
#define CPUID_VENDOR_ZHAOXIN       "  Shanghai  "
#define CPUID_VENDOR_HYGON         "HygonGenuine"
#define CPUID_VENDOR_ELBRUS        "E2K MACHINE "
#define CPUID_VENDOR_QEMU          "TCGTCGTCGTCG"
#define CPUID_VENDOR_KVM           " KVMKVMKVM  "
#define CPUID_VENDOR_VMWARE        "VMwareVMware"
#define CPUID_VENDOR_VIRTUALBOX    "VBoxVBoxVBox"
#define CPUID_VENDOR_XEN           "XenVMMXenVMM"
#define CPUID_VENDOR_HYPERV        "Microsoft Hv"
#define CPUID_VENDOR_PARALLELS     " prl hyperv "
#define CPUID_VENDOR_PARALLELS_ALT " lrpepyh vr "
#define CPUID_VENDOR_BHYVE         "bhyve bhyve "
#define CPUID_VENDOR_QNX           " QNXQVMBSQG "

/* When CPUID is called with RAX equal to zero, a bit field
 *   is returned in EDX with the following mappings.
 * Different brands of CPUs may give different meanings to these.
 * Recent processors return to ECX, while on older CPUs this will likely be garbage.
 */
enum class CPUID_FEATURE : unsigned {
    ECX_SSE3         = 1u << 0,
    ECX_PCLMUL       = 1u << 1,
    ECX_DTES64       = 1u << 2,
    ECX_MONITOR      = 1u << 3,
    ECX_DS_CPL       = 1u << 4,
    ECX_VMX          = 1u << 5,
    ECX_SMX          = 1u << 6,
    ECX_EST          = 1u << 7,
    ECX_TM2          = 1u << 8,
    ECX_SSSE3        = 1u << 9,
    ECX_CID          = 1u << 10,
    ECX_SDBG         = 1u << 11,
    ECX_FMA          = 1u << 12,
    ECX_CX16         = 1u << 13,
    ECX_XTPR         = 1u << 14,
    ECX_PDCM         = 1u << 15,
    ECX_PCID         = 1u << 17,
    ECX_DCA          = 1u << 18,
    ECX_SSE4_1       = 1u << 19,
    ECX_SSE4_2       = 1u << 20,
    ECX_X2APIC       = 1u << 21,
    ECX_MOVBE        = 1u << 22,
    ECX_POPCNT       = 1u << 23,
    ECX_TSC          = 1u << 24,
    ECX_AES          = 1u << 25,
    ECX_XSAVE        = 1u << 26,
    ECX_OSXSAVE      = 1u << 27,
    ECX_AVX          = 1u << 28,
    ECX_F16C         = 1u << 29,
    ECX_RDRAND       = 1u << 30,
    ECX_HYPERVISOR   = 1u << 31,

    EDX_FPU          = 1u << 0,
    EDX_VME          = 1u << 1,
    EDX_DE           = 1u << 2,
    EDX_PSE          = 1u << 3,
    EDX_TSC          = 1u << 4,
    EDX_MSR          = 1u << 5,
    EDX_PAE          = 1u << 6,
    EDX_MCE          = 1u << 7,
    EDX_CX8          = 1u << 8,
    EDX_APIC         = 1u << 9,
    EDX_SEP          = 1u << 11,
    EDX_MTRR         = 1u << 12,
    EDX_PGE          = 1u << 13,
    EDX_MCA          = 1u << 14,
    EDX_CMOV         = 1u << 15,
    EDX_PAT          = 1u << 16,
    EDX_PSE36        = 1u << 17,
    EDX_PSN          = 1u << 18,
    EDX_CLFLUSH      = 1u << 19,
    EDX_DS           = 1u << 21,
    EDX_ACPI         = 1u << 22,
    EDX_MMX          = 1u << 23,
    EDX_FXSR         = 1u << 24,
    EDX_SSE          = 1u << 25,
    EDX_SSE2         = 1u << 26,
    EDX_SS           = 1u << 27,
    EDX_HTT          = 1u << 28,
    EDX_TM           = 1u << 29,
    EDX_IA64         = 1u << 30,
    EDX_PBE          = 1u << 31,
};

#endif /* LENSOR_OS_CPUID_H */
