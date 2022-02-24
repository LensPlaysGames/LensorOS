#ifndef LENSOR_OS_CPU_H 
#define LENSOR_OS_CPU_H

#include "integers.h"
#include "linked_list.h"

/* Resources:
 * |- https://wiki.osdev.org/Multiprocessing#NUMA_.28Non-Uniform_Memory_Access.29
 *
 */

/* CPUDescription holds the data common to all CPUs present on the system
 *   This includes things like Vendor, Brand, Cache Sizes, Feature Flags, etc. 
 */
class CPUDescription {
    // Forward declaration
    class CPU;
public:
    void set_fxsr_capable() { FXSRCapable = true; };
    void set_fxsr_enabled() { FXSREnabled = true; };

    void set_fpu_capable() { FPUCapable = true; };
    void set_fpu_enabled() { FPUEnabled = true; };

    void set_sse_capable() { SSECapable = true; };
    void set_sse_enabled() { SSEEnabled = true; };

    void set_avx_capable() { AVXCapable = true; };
    void set_avx_enabled() { AVXEnabled = true; };

private:
    /* CPU Feature Flags
     *   Once enabled, can not be disabled.
     */
    bool FXSRCapable { false };
    bool FXSREnabled { false };
    bool FPUCapable { false };
    bool FPUEnabled { false };
    bool SSECapable { false };
    bool SSEEnabled { false };
    bool AVXCapable { false };
    bool AVXEnabled { false };
    /* Pointer to c-style string containing the name of the CPU Vendor */
    char* VendorID { nullptr };
    /* List of central processing units (why call them central anymore??) */
    SinglyLinkedList<CPU> CPUs;
};

class CPU {
    friend class CPUDescription;
public:
    
private:
    CPUDescription* Description;
    u16 LogicalCore;
    u16 Core;
    // See non-uniform memory access resources above.
    u16 NUMAChip;
    u16 NUMADomain;
    // Advanced Programmable Interrupt Controller Identifier.
    // Different for each CPU; found within ACPI MADT table.
    u16 APICID;
};

#endif /* LENSOR_OS_CPU_H */
