#ifndef LENSOR_OS_SCHEDULER_H
#define LENSOR_OS_SCHEDULER_H

#include "integers.h"
#include "interrupts/interrupts.h"

namespace Memory {
    class PageTable;
}

struct CPUState;
/// External symbols for 'scheduler.asm', defined in `scheduler.cpp`
extern void(*scheduler_switch_task)(CPUState*);
extern void(*timer_tick)();
/// Interrupt handler function found in `scheduler.asm`
extern "C" void irq0_handler();

/* THE KERNEL PROCESS SCHEDULER
 * | Hijack the System Timer Interrupt to switch processes, if needed.
 * `- Each Kernel Process needs:
 *    |- To save register states
 *    |- To store an address of page table
 *    `- To store an address of the next process, if it exists (singly-linked list).
 */

// FIXME: Take into account different CPU architectures.
struct CPUState {
    u64 RSP;
    u64 RBX;
    u64 RCX;
    u64 RDX;
    u64 RSI;
    u64 RDI;
    u64 RBP;
    u64 R8;
    u64 R9;
    u64 R10;
    u64 R11;
    u64 R12;
    u64 R13;
    u64 R14;
    u64 R15;
    u64 FS;
    u64 GS;
    u64 RAX;
    InterruptFrame Frame;
} __attribute__((packed));

struct KernelProcess {
    CPUState CPU;
    Memory::PageTable* CR3;
    KernelProcess* Next;
};

// The proess that acts as the beginning thread.
extern KernelProcess StartupProcess;

/// The scheduler handles multi-tasking within the kernel.
class Scheduler {
    /* TODO: I don't know if these go here but I do need them somewhere at some point.
     * |- load_binary() that will read a flat binary and add it to process list.
     * `- load_elf() that will read an elf executable and add it to process list.
     */
public:
    static void initialize();
    /* Switch to the next available task.
     * | Called by IRQ0 Handler (System Timer Interrupt).
     * |- Copy registers saved from IRQ0 to current process.
     * |- Update current process to next available process.
     * `- Manipulate stack to trick `iretq` into doing what we want.
     */
    static void switch_process(CPUState*);
    static void add_kernel_process(KernelProcess*);

private:
    static KernelProcess* ProcessQueue;
    static KernelProcess* CurrentProcess;
};

#endif
