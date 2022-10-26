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

/* TODO: Take into account different CPU architectures.
 *  This can be done by including ${ARCH}/ directory
 *  with the build system, and putting this there.
 */
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

typedef u64 pid_t;

/* TODO:
 * |-- File Descriptor Table (Dynamic list of process' open file descriptors).
 * `-- As only processes should make syscalls, should syscalls be defined in terms of process?
 */
struct Process {
    pid_t ProcessID;
    Memory::PageTable* CR3 { nullptr };
    CPUState CPU;
};

/// External symbols for 'scheduler.asm', defined in `scheduler.cpp`
extern void(*scheduler_switch_process)(CPUState*)
    __attribute__((no_caller_saved_registers));
extern void(*timer_tick)();

namespace Scheduler {
    /// External symbol defined in `scheduler.cpp`
    // The list node of the currently executing process.
    extern SinglyLinkedListNode<Process*>* CurrentProcess;

    bool initialize();

    /// Get a process ID number that is unique.
    pid_t request_pid();

    /* Switch to the next available task.
     * | Called by IRQ0 Handler (System Timer Interrupt).
     * |-- Copy registers saved from IRQ0 to current process.
     * |-- Update current process to next available process.
     * `-- Manipulate stack to trick `iretq` into doing what we want.
     */
    void switch_process(CPUState*);

    /// Add an existing process to the list of processes.
    void add_process(Process*);

    /// remove the process with PID list of processes.
    bool remove_process(pid_t);

    void print_debug();
}

__attribute__((no_caller_saved_registers))
void scheduler_switch(CPUState*);

#endif
