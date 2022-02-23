#ifndef LENSOR_OS_CPUID_H
#define LENSOR_OS_CPUID_H

/* Many thanks to osdev wiki: https://wiki.osdev.org/CPUID */

#include "integers.h"

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
enum class CPUID_FEATURE {
    ECX_SSE3         = 1 << 0, 
    ECX_PCLMUL       = 1 << 1,
    ECX_DTES64       = 1 << 2,
    ECX_MONITOR      = 1 << 3,  
    ECX_DS_CPL       = 1 << 4,  
    ECX_VMX          = 1 << 5,  
    ECX_SMX          = 1 << 6,  
    ECX_EST          = 1 << 7,  
    ECX_TM2          = 1 << 8,  
    ECX_SSSE3        = 1 << 9,  
    ECX_CID          = 1 << 10,
    ECX_SDBG         = 1 << 11,
    ECX_FMA          = 1 << 12,
    ECX_CX16         = 1 << 13, 
    ECX_XTPR         = 1 << 14, 
    ECX_PDCM         = 1 << 15, 
    ECX_PCID         = 1 << 17, 
    ECX_DCA          = 1 << 18, 
    ECX_SSE4_1       = 1 << 19, 
    ECX_SSE4_2       = 1 << 20, 
    ECX_X2APIC       = 1 << 21, 
    ECX_MOVBE        = 1 << 22, 
    ECX_POPCNT       = 1 << 23, 
    ECX_TSC          = 1 << 24, 
    ECX_AES          = 1 << 25, 
    ECX_XSAVE        = 1 << 26, 
    ECX_OSXSAVE      = 1 << 27, 
    ECX_AVX          = 1 << 28,
    ECX_F16C         = 1 << 29,
    ECX_RDRAND       = 1 << 30,
    ECX_HYPERVISOR   = 1 << 31,
 
    EDX_FPU          = 1 << 0,  
    EDX_VME          = 1 << 1,  
    EDX_DE           = 1 << 2,  
    EDX_PSE          = 1 << 3,  
    EDX_TSC          = 1 << 4,  
    EDX_MSR          = 1 << 5,  
    EDX_PAE          = 1 << 6,  
    EDX_MCE          = 1 << 7,  
    EDX_CX8          = 1 << 8,  
    EDX_APIC         = 1 << 9,  
    EDX_SEP          = 1 << 11, 
    EDX_MTRR         = 1 << 12, 
    EDX_PGE          = 1 << 13, 
    EDX_MCA          = 1 << 14, 
    EDX_CMOV         = 1 << 15, 
    EDX_PAT          = 1 << 16, 
    EDX_PSE36        = 1 << 17, 
    EDX_PSN          = 1 << 18, 
    EDX_CLFLUSH      = 1 << 19, 
    EDX_DS           = 1 << 21, 
    EDX_ACPI         = 1 << 22, 
    EDX_MMX          = 1 << 23, 
    EDX_FXSR         = 1 << 24, 
    EDX_SSE          = 1 << 25,
    EDX_SSE2         = 1 << 26, 
    EDX_SS           = 1 << 27, 
    EDX_HTT          = 1 << 28, 
    EDX_TM           = 1 << 29, 
    EDX_IA64         = 1 << 30,
    EDX_PBE          = 1 << 31
};

#endif /* LENSOR_OS_CPUID_H */
