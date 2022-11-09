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

#include <scheduler.h>

#include <debug.h>
#include <integers.h>
#include <interrupts/idt.h>
#include <interrupts/interrupts.h>
#include <linked_list.h>
#include <memory.h>
#include <memory/paging.h>
#include <memory/virtual_memory_manager.h>
#include <pit.h>
#include <tss.h>
#include <uart.h>

/// External symbol definitions for `scheduler.asm`
void(*scheduler_switch_process)(CPUState*)
    __attribute__((no_caller_saved_registers));

void(*timer_tick)();

namespace Scheduler {
    // Not the best, but wouldn't be a problem unless ridiculous uptime.
    pid_t the_pid { 0 };
    pid_t request_pid() {
        return the_pid++;
    }

    Process StartupProcess;

    SinglyLinkedList<Process*>* ProcessQueue { nullptr };
    SinglyLinkedListNode<Process*>* CurrentProcess { nullptr };

    void print_debug() {
        dbgmsg_s("[SCHED]: Debug information:\r\n"
                 "  Process Queue:\r\n");
        ProcessQueue->for_each([](auto* it) {
            Process& process = *it->value();
            dbgmsg("    Process %ull at %x\r\n"
                   "      CR3:      %x\r\n"
                   "      RAX:      %x\r\n"
                   "      RBX:      %x\r\n"
                   "      RCX:      %x\r\n"
                   "      RDX:      %x\r\n"
                   "      RSI:      %x\r\n"
                   "      RDI:      %x\r\n"
                   "      RBP:      %x\r\n"
                   "      RSP:      %x\r\n"
                   "      R8:       %x\r\n"
                   "      R9:       %x\r\n"
                   "      R10:      %x\r\n"
                   "      R11:      %x\r\n"
                   "      R12:      %x\r\n"
                   "      R13:      %x\r\n"
                   "      R14:      %x\r\n"
                   "      R15:      %x\r\n"
                   "      Frame:\r\n"
                   "        RIP:    %x\r\n"
                   "        CS:     %x\r\n"
                   "        RFLAGS: %x\r\n"
                   "        RSP:    %x\r\n"
                   "        SS:     %x\r\n"
                   , process.ProcessID
                   , &process
                   , process.CR3
                   , process.CPU.RAX
                   , process.CPU.RBX
                   , process.CPU.RCX
                   , process.CPU.RDX
                   , process.CPU.RSI
                   , process.CPU.RDI
                   , process.CPU.RBP
                   , process.CPU.RSP
                   , process.CPU.R8
                   , process.CPU.R9
                   , process.CPU.R10
                   , process.CPU.R11
                   , process.CPU.R12
                   , process.CPU.R13
                   , process.CPU.R14
                   , process.CPU.R15
                   , process.CPU.Frame.ip
                   , process.CPU.Frame.cs
                   , process.CPU.Frame.flags
                   , process.CPU.Frame.sp
                   , process.CPU.Frame.ss
                   );
            dbgmsg("      File Descriptors:\r\n");
            for (const auto& [procfd, fd] : process.FileDescriptors.pairs()) {
                dbgmsg("        %ull -> SysFD %ull\r\n", u64(procfd), u64(fd));
            }
        });
        dbgmsg_s("\r\n");
    }

    void add_process(Process* process) {
        process->ProcessID = request_pid();
        ProcessQueue->add_end(process);
        dbgmsg_s("[SCHED]: Added process.\r\n");
        print_debug();
    }

    bool remove_process(pid_t pid) {
        Process* processToRemove = nullptr;
        int processToRemoveIndex = 0;
        ProcessQueue->for_each([pid, &processToRemove, &processToRemoveIndex](auto* it) {
            Process* process = it->value();
            if (process->ProcessID == pid) {
                processToRemove = process;
            } else if (processToRemove == nullptr) {
                processToRemoveIndex += 1;
            }
        });
        if (processToRemove) {
            ProcessQueue->remove(processToRemoveIndex);
            processToRemove->destroy();
            return true;
        }
        return false;
    }

    bool initialize() {
        // IRQ handler in assembly increments PIT ticks counter using this function.
        timer_tick = pit_tick;
        // IRQ handler in assembly switches processes using this function.
        scheduler_switch_process = scheduler_switch;
        // Setup currently executing code as the start process.
        StartupProcess.CR3 = Memory::active_page_map();
        // Create the process queue and add the startup process to it.
        ProcessQueue = new SinglyLinkedList<Process*>;
        if (ProcessQueue == nullptr) {
            dbgmsg_s("\033[31mScheduler failed to initialize:\033[0m Could not create process list.\r\n");
            return false;
        }
        add_process(&StartupProcess);
        CurrentProcess = ProcessQueue->head();
        // Install IRQ0 handler found in `scheduler.asm` (over-write default system timer handler).
        gIDT.install_handler((u64)irq0_handler, PIC_IRQ0);
        gIDT.flush();
        dbgmsg_s("Flushed IDT after installing new IRQ0 handler\r\n");
        return true;
    }

    enum class IncludeGivenProcess {
        Yes = 0,
        No = 1,
    };

    inline SinglyLinkedListNode<Process*>* find_next_viable_process_after
    (SinglyLinkedListNode<Process*>* startProcess
     , IncludeGivenProcess include = IncludeGivenProcess::No)
    {
        auto* NextProcess = startProcess;
        while (NextProcess != nullptr)
        {
            if (include == IncludeGivenProcess::Yes) {
                NextProcess = startProcess;
                include = IncludeGivenProcess::No;
            }
            else NextProcess = NextProcess->next();
            if (NextProcess != nullptr && NextProcess->value()->State == Process::RUNNING)
                break;
        }
        return NextProcess;
    }

    SinglyLinkedListNode<Process*>* next_viable_process
    (SinglyLinkedListNode<Process*>* startProcess
     , IncludeGivenProcess include = IncludeGivenProcess::No)
    {
        auto* NextProcess = find_next_viable_process_after(startProcess, include);
        if (NextProcess == nullptr) {
            NextProcess = find_next_viable_process_after(ProcessQueue->head(), IncludeGivenProcess::Yes);
            if (NextProcess == nullptr) {
                // Process list has zero viable processes.
                // FIXME: I honestly don't know how to handle this correctly.
                // Hang forever.
                dbgmsg("[SCHED]: I don't know what to do when there are no processes!\r\n");
                while (true)
                    asm ("hlt");
            }
        }
        return NextProcess;
    }

    /// Called from `irq0_handler` in `scheduler.asm`
    /// A stupid simple round-robin process switcher.
    void switch_process(CPUState* cpu) {
        memcpy(cpu, &CurrentProcess->value()->CPU, sizeof(CPUState));
        // Handle single viable process or end of queue.
        if (CurrentProcess->next() == nullptr) {
            // If there is only one viable process,
            // this is a short-cut to do nothing.
            if(CurrentProcess == ProcessQueue->head())
                return;

            // Otherwise, we are at the end of the queue,
            // and must reset back to the beginning of it.
            CurrentProcess = next_viable_process(ProcessQueue->head()
                                                 , IncludeGivenProcess::Yes);
        }
        else CurrentProcess = next_viable_process(CurrentProcess);
        // Update state of CPU that will be restored.
        memcpy(&CurrentProcess->value()->CPU, cpu, sizeof(CPUState));
        // Use new process' page map.
        Memory::flush_page_map(CurrentProcess->value()->CR3);
        // Update ES and DS to SS.
        asm("xor %%rax, %%rax\r\n\t"
            "movq %0, %%rax\r\n\t"
            "movw %%ax, %%es\r\n\t"
            "movw %%ax, %%ds\r\n\t"
            :: "r"(cpu->Frame.ss)
            : "rax"
            );
        // Update FS and GS to SS.
        // Technically we could use these for whatever.
        cpu->FS = cpu->Frame.ss;
        cpu->GS = cpu->Frame.ss;
    }
}
