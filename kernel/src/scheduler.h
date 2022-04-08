#ifndef LENSOR_OS_SCHEDULER_H
#define LENSOR_OS_SCHEDULER_H

#include <integers.h>
#include <interrupts/interrupts.h>
#include <linked_list.h>

namespace Memory {
    class PageTable;
}

/// Interrupt handler function found in `scheduler.asm`
extern "C" void irq0_handler();

/* THE KERNEL PROCESS SCHEDULER
 * | Hijack the System Timer Interrupt to switch processes, if needed.
 * `- Each Kernel Process needs:
 *    |- To save register states (TODO: take advantage of `xsave`, `fxsave`, etc.)
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

/* TODO:
 * |-- File Descriptor Table (Dynamic list of process' open file descriptors).
 * |-- Some sort of priority level system? Kernel tasks get highest, userspace medium, idle lowest?
 * `-- As only processes should make syscalls, should syscalls be defined in terms of process?
 */
struct Process {
    CPUState CPU;
    Memory::PageTable* CR3;
};

/// External symbols for 'scheduler.asm', defined in `scheduler.cpp`
extern void* scheduler_switch_process;
extern void(*timer_tick)();

namespace Scheduler {
    extern SinglyLinkedListNode<Process*>* CurrentProcess;

    bool initialize();

    /* Switch to the next available task.
     * | Called by IRQ0 Handler (System Timer Interrupt).
     * |- Copy registers saved from IRQ0 to current process.
     * |- Update current process to next available process.
     * `- Manipulate stack to trick `iretq` into doing what we want.
     */
    void switch_process(CPUState*);

    // Add an existing process to the list of processes.
    void add_process(Process*);
}

#endif
