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
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <memory/paging.h>
#include <memory/region.h>
#include <storage/file_metadata.h>
#include <memory>
#include <vector>
#include <extensions>
#include <vfs_forward.h>

#include <x86_64/cpu.h>

namespace Memory {
    class PageTable;
}

/// Interrupt handler function found in `scheduler.asm`
extern "C" void irq0_handler();

typedef u64 pid_t;

struct ZombieState {
    pid_t PID;
    int ReturnStatus;
};

struct Process {
    pid_t ProcessID = 0;

    enum ProcessState {
        RUNNING,
        SLEEPING,
    } State = RUNNING;

    /// Keep track of memory that should be freed when the process exits.
    SinglyLinkedList<Memory::Region> Memories;
    usz next_region_vaddr = 0xf8000000;

    pid_t ParentProcess{(pid_t)-1};

    /// A list of programs waiting to be set to `RUNNING` when this
    /// program exits.
    std::vector<pid_t> WaitingList;

    // Information regarding child processes that have exited or
    // inherited from a child that has exited. See waitpid syscall.
    std::vector<ZombieState> Zombies;

    /// Keep track of opened files that may be freed when the process
    /// exits, if no other process has it open.
    std::sparse_vector<SysFD, SysFD::Invalid, ProcFD> FileDescriptors;

    std::string ExecutablePath { "" };
    std::string WorkingDirectory { "" };

    /// Used to save/restore CPU state when a context switch occurs.
    CPUState CPU;

    /// TODO: We need to figure out how each architecture can affect the
    /// process structure. Maybe defines for each function definition,
    /// or just leave the bulk of the process implementation to the
    /// architecture and have a simpler ProcessBase that is inherited
    /// from.

    /* Data for extra CPU info (fxsave, etc). */
    /* NOTE: fxsave and friends leave bytes 464:511 available for software use. */
    alignas(16) u8 CPUExtra[512] = {0};
    bool CPUExtraSet = false;

    Memory::PageTable* CR3 { nullptr };

    Process() = default;

    /// Processes are not copyable.
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    // size is in bytes.
    void add_memory_region(void* vaddr, void* paddr, usz size, u64 flags) {
        Memories.add({vaddr, paddr, size, flags});
    }

    void add_memory_region(const Memory::Region& memory) {
        Memories.add({memory.vaddr, memory.paddr, memory.length, memory.flags});
    }

    /// Find region in memories by vaddr and remove it.
    void remove_memory_region(void* vaddr) {
        usz index = 0;
        auto* region_it = Memories.head();
        for (; region_it; region_it = region_it->next()) {
            if (region_it->value().vaddr == vaddr) {
                break;
            }
            ++index;
        }
        if (region_it) {
            Memories.remove(index);
        }
    }

    /// Set the return value within CPU state.
    void set_return_value(usz value) {
        CPU.RAX = value;
    }

    /// Set the process state to running.
    /// @param setReturn
    ///   When true, update the CPU state before running so that the
    ///   process will see the return value given as it's return value.
    ///   On x86_64, just sets RAX.
    /// @param returnValue
    ///   Used iff setReturn is true.
    ///   The value that the process being run will see as the return
    ///   value.
    ///
    /// Examples:
    /// `unblock()` simply sets the process running, no change to
    /// existing CPU state.
    /// `unblock(false)` and `unblock(false, x)` are both equal to
    /// passing zero arguments, the case just above.
    /// `unblock(true)` will set the return value to 0, as well as set
    /// the process running.
    /// `unblock(true, x)` will set the return value to x, as well as
    /// set the process running.
    void unblock(bool setReturn = false, usz returnValue = 0) {
        if (setReturn) set_return_value(returnValue);
        State = RUNNING;
    }

    /// @param status Relays exit status to all waiting processes (i.e. via `waitpid`).
    void destroy(int status);
};

/// External symbols for 'scheduler.asm', defined in `scheduler.cpp`
extern void(*scheduler_switch_process)(CPUState*)
    __attribute__((no_caller_saved_registers));
extern void(*timer_tick)();

namespace Scheduler {
    /// External symbol defined in `scheduler.cpp`
    // The list node of the currently executing process.
    extern SinglyLinkedListNode<Process*>* CurrentProcess;

    extern std::vector<Memory::PageTable*> PageMapsToFree;

    bool initialize();

    /// Get a process ID number that is unique.
    pid_t request_pid();

    /// Get the process with PID if it is within list of processes, otherwise return NULL.
    Process* process(pid_t);

    /* Switch to the next available task.
     * | Called by IRQ0 Handler (System Timer Interrupt).
     * |-- Copy registers saved from IRQ0 to current process.
     * |-- Update current process to next available process.
     * `-- Manipulate stack to trick `iretq` into doing what we want.
     */
    void switch_process(CPUState*);

    /// Add an existing process to the list of processes.
    /// Creates and assigns a unique PID.
    pid_t add_process(Process*);

    Process* last_process();

    /// Remove the process with PID from the scheduler's list of viable
    /// processes to switch to. If not found, do nothing. Destroy the process.
    /// NOTE: If passing pid of current process, be careful to stay in
    /// kernel until calling yield. DO NOT try to return to a destroyed
    /// process.
    ///
    /// @param status
    ///     Used for relaying status to processes
    ///     waiting on this process (i.e. via `waitpid`)
    ///
    /// @return true iff process with given PID is found, removed, and destroyed.
    bool remove_process(pid_t, int status);

    void print_debug();

    /// Stop the current process, and start the next. NOTE: CPU state
    /// is not saved by this function, so be sure the saved process CPU
    /// state is valid and ready to be returned to.
    [[noreturn]] void yield();

    // Call `map_pages` with the given data on every process in the
    // process queue.
    void map_pages_in_all_processes
    (void* virtualAddress
     , void* physicalAddress
     , u64 mappingFlags
     , size_t pages
     , Memory::ShowDebug d = Memory::ShowDebug::No);
}

__attribute__((no_caller_saved_registers))
void scheduler_switch(CPUState*);

pid_t CopyUserspaceProcess(Process* original);

#endif
