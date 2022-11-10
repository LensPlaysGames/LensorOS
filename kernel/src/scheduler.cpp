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

#include <format>

#include <debug.h>
#include <integers.h>
#include <interrupts/idt.h>
#include <interrupts/interrupts.h>
#include <linked_list.h>
#include <memory.h>
#include <memory/paging.h>
#include <memory/virtual_memory_manager.h>
#include <pit.h>
#include <scheduler.h>
#include <tss.h>
#include <uart.h>
#include <virtual_filesystem.h>

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
        std::print("[SCHED]: Debug information:\r\n"
                 "  Process Queue:\r\n");
        ProcessQueue->for_each([](auto* it) {
            Process& process = *it->value();
            std::print("    Process {} at {}\r\n"
                       "      CR3:      {}\r\n"
                       "      RAX:      {:#016x}\r\n"
                       "      RBX:      {:#016x}\r\n"
                       "      RCX:      {:#016x}\r\n"
                       "      RDX:      {:#016x}\r\n"
                       "      RSI:      {:#016x}\r\n"
                       "      RDI:      {:#016x}\r\n"
                       "      RBP:      {:#016x}\r\n"
                       "      RSP:      {:#016x}\r\n"
                       "      R8:       {:#016x}\r\n"
                       "      R9:       {:#016x}\r\n"
                       "      R10:      {:#016x}\r\n"
                       "      R11:      {:#016x}\r\n"
                       "      R12:      {:#016x}\r\n"
                       "      R13:      {:#016x}\r\n"
                       "      R14:      {:#016x}\r\n"
                       "      R15:      {:#016x}\r\n"
                       "      Frame:\r\n"
                       "        RIP:    {:#016x}\r\n"
                       "        CS:     {:#016x}\r\n"
                       "        RFLAGS: {:#016x}\r\n"
                       "        RSP:    {:#016x}\r\n"
                       "        SS:     {:#016x}\r\n"
                       , process.ProcessID, (void*) &process
                       , (void*) process.CR3
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
            std::print("      File Descriptors:\r\n");
            for (const auto& [procfd, fd] : process.FileDescriptors.pairs()) {
                std::print("        {} -> {}\r\n", procfd, fd);
            }
        });
        std::print("\r\n");
    }

    void add_process(Process* process) {
        process->ProcessID = request_pid();
        ProcessQueue->add_end(process);
        std::print("[SCHED]: Added process.\r\n");
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
            // TODO: Actually de-allocate memory!!!
            ProcessQueue->remove(processToRemoveIndex);
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
            std::print("\033[31mScheduler failed to initialize:\033[0m Could not create process list.\r\n");
            return false;
        }
        add_process(&StartupProcess);
        CurrentProcess = ProcessQueue->head();
        // Install IRQ0 handler found in `scheduler.asm` (over-write default system timer handler).
        gIDT.install_handler((u64)irq0_handler, PIC_IRQ0);
        gIDT.flush();
        std::print("Flushed IDT after installing new IRQ0 handler\r\n");
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
                std::print("[SCHED]: I don't know what to do when there are no processes!\r\n");
                while (true)
                    asm ("hlt");
            }
        }
        return NextProcess;
    }

    /// Called from `irq0_handler` in `scheduler.asm`
    /// A stupid simple round-robin process switcher.
    void switch_process(CPUState* cpu) {
        memcpy(&CurrentProcess->value()->CPU, cpu, sizeof(CPUState));
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
        memcpy(cpu, &CurrentProcess->value()->CPU, sizeof(CPUState));
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
