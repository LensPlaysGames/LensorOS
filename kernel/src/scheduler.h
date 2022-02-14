#ifndef LENSOR_OS_SCHEDULER_H
#define LENSOR_OS_SCHEDULER_H

#include "integers.h"
#include "interrupts/interrupts.h"

extern void* scheduler_switch_task;
extern u64* timer_ticks;

/// Interrupt handler function found in `scheduler.asm`
extern "C" void irq0_handler();

class PageTable;


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
    PageTable* CR3;
    KernelProcess* Next;
};

extern KernelProcess StartupProcess;

namespace Scheduler {
    /* TODO:
     * |- load_binary() that will read a flat binary and add it to process list.
     * `- load_elf() that will read an elf executable and add it to process list.
     */

    // ProcessQueue and CurrentProcess can NOT be a
    //   nullptr, or things will definitely break.
    extern KernelProcess* ProcessQueue;
    extern KernelProcess* CurrentProcess;

    void add_task(KernelProcess*);
    
    /* Switch to the next available task.
     * | Called by IRQ0 Handler (System Timer Interrupt).
     * |- Copy registers saved from IRQ0 to current process.
     * |- Update current process to next available process.
     * `- Manipulate stack to trick `iretq` into doing what we want.
     */
    void switch_task(CPUState*);

    void Initialize(PageTable*);
};

#endif
