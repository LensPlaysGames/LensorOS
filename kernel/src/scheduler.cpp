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

#include <integers.h>
#include <interrupts/idt.h>
#include <interrupts/interrupts.h>
#include <linked_list.h>
#include <memory.h>
#include <memory/paging.h>
#include <memory/virtual_memory_manager.h>
#include <pit.h>
#include <scheduler.h>
#include <vfs_forward.h>
#include <system.h>

/// External symbol definitions for `scheduler.asm`
void(*scheduler_switch_process)(CPUState*)
    __attribute__((no_caller_saved_registers));

void(*timer_tick)();

void Process::destroy() {
    // Run all of the programs in the WaitingList.
    for(pid_t pid : WaitingList) {
        Process *waitingProcess = Scheduler::process(pid);
        if (waitingProcess) {
            waitingProcess->State = Process::ProcessState::RUNNING;
        }
    }
    // Free memory regions.
    Memories.for_each([](SinglyLinkedListNode<Memory::Region>* it){
        Memory::free_pages(it->value().paddr, it->value().pages);
    });
    // Close open files.
    // NOTE: There *should* be none; libc should close all open files on destruction.
    for (const auto& [procfd, fd] : FileDescriptors.pairs()) {
        SYSTEM->virtual_filesystem().close(this, procfd);
    }

    // TODO: Free page table? May want to wait until another process
    // can do it, just in case we are still in this process. Or we have
    // to flush a new page table before we call destroy.
}

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
        std::print("[SCHED]: Debug information:\n"
                 "  Process Queue:\n");
        ProcessQueue->for_each([](auto* it) {
            Process& process = *it->value();
            std::print("    Process {} at {}\n"
                       "      CR3:      {}\n"
                       "      RAX:      {:#016x}\n"
                       "      RBX:      {:#016x}\n"
                       "      RCX:      {:#016x}\n"
                       "      RDX:      {:#016x}\n"
                       "      RSI:      {:#016x}\n"
                       "      RDI:      {:#016x}\n"
                       "      RBP:      {:#016x}\n"
                       "      RSP:      {:#016x}\n"
                       "      R8:       {:#016x}\n"
                       "      R9:       {:#016x}\n"
                       "      R10:      {:#016x}\n"
                       "      R11:      {:#016x}\n"
                       "      R12:      {:#016x}\n"
                       "      R13:      {:#016x}\n"
                       "      R14:      {:#016x}\n"
                       "      R15:      {:#016x}\n"
                       "      Frame:\n"
                       "        RIP:    {:#016x}\n"
                       "        CS:     {:#016x}\n"
                       "        RFLAGS: {:#016x}\n"
                       "        RSP:    {:#016x}\n"
                       "        SS:     {:#016x}\n"
                       , process.ProcessID, (void*) &process
                       , (void*) process.CR3
                       , u64(process.CPU.RAX)
                       , u64(process.CPU.RBX)
                       , u64(process.CPU.RCX)
                       , u64(process.CPU.RDX)
                       , u64(process.CPU.RSI)
                       , u64(process.CPU.RDI)
                       , u64(process.CPU.RBP)
                       , u64(process.CPU.RSP)
                       , u64(process.CPU.R8)
                       , u64(process.CPU.R9)
                       , u64(process.CPU.R10)
                       , u64(process.CPU.R11)
                       , u64(process.CPU.R12)
                       , u64(process.CPU.R13)
                       , u64(process.CPU.R14)
                       , u64(process.CPU.R15)
                       , u64(process.CPU.Frame.ip)
                       , u64(process.CPU.Frame.cs)
                       , u64(process.CPU.Frame.flags)
                       , u64(process.CPU.Frame.sp)
                       , u64(process.CPU.Frame.ss)
                       );
            std::print("      File Descriptors:\n");
            for (const auto& [procfd, fd] : process.FileDescriptors.pairs()) {
                std::print("        {} -> {}\n", s64(procfd), s64(fd));
            }
        });
        std::print("\n");
    }

    Process* process(pid_t pid) {
        for (SinglyLinkedListNode<Process*>* it = Scheduler::ProcessQueue->head(); it; it = it->next()) {
            if (it->value()->ProcessID == pid) {
                return it->value();
            }
        }
        return nullptr;
    }

    Process* last_process() {
        return ProcessQueue->tail()->value();
    }

    pid_t add_process(Process* process) {
        pid_t pid = request_pid();
        process->ProcessID = pid;
        ProcessQueue->add_end(process);
        //std::print("[SCHED]: Added process.\n");
        //print_debug();
        return pid;
    }

    bool remove_process(pid_t pid) {
        Process* processToRemove = nullptr;
        int processToRemoveIndex = 0;
        for (SinglyLinkedListNode<Process*>* it = ProcessQueue->head(); it; it = it->next()) {
            Process* process = it->value();
            if (process->ProcessID == pid) {
                processToRemove = process;
                break;
            }
            processToRemoveIndex += 1;
        }
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
            std::print("\033[31mScheduler failed to initialize:\033[0m Could not create process list.\n");
            return false;
        }
        add_process(&StartupProcess);
        CurrentProcess = ProcessQueue->head();
        // Install IRQ0 handler found in `scheduler.asm` (over-write default system timer handler).
        gIDT.install_handler((u64)irq0_handler, PIC_IRQ0);
        gIDT.flush();
        std::print("Flushed IDT after installing new IRQ0 handler\n");
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
                std::print("[SCHED]: I don't know what to do when there are no processes!\n");
                while (true)
                    asm ("hlt");
            }
        }
        return NextProcess;
    }

    void switch_process_impl(CPUState *cpu) {
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
        asm("xor %%rax, %%rax\n\t"
            "movq %0, %%rax\n\t"
            "movw %%ax, %%es\n\t"
            "movw %%ax, %%ds\n\t"
            :: "r"(cpu->Frame.ss)
            : "rax"
            );
        // Eventually, FS and GS will be used for TLS, or Thread Local
        // Storage.
        // Update FS and GS to SS.
        cpu->FS = cpu->Frame.ss;
        cpu->GS = cpu->Frame.ss;
    }

    /// Called from `irq0_handler` in `scheduler.asm`
    /// A stupid simple round-robin process switcher.
    void switch_process(CPUState* cpu) {
        memcpy(&CurrentProcess->value()->CPU, cpu, sizeof(CPUState));

        // TODO: Save extra context depending on system features
        // (i.e. xmm registers with fxsave/fxrestore)
        // I will be curious as to where we store the buffers for these;
        // a member in Process seems a little platform-dependant.

        // TODO: Check all processes that called `wait(ms)`, and run/
        // unstop them if the timestamp is greater than the calculated
        // one.

        switch_process_impl(cpu);
    }

    // Defined in `scheduler.asm`
    extern "C" void yield_asm(CPUState*);

    void yield() {
        CPUState newstate;
        switch_process_impl(&newstate);
        // iretq to the new process, bb.
        yield_asm(&newstate);
    }
}

pid_t CopyUserspaceProcess(Process* original) {
    // Copy current page table (fork)
    auto* newPageTable = Memory::clone_page_map(original->CR3);
    if (newPageTable == nullptr) {
        std::print("Failed to clone current page map for new process page map.\n");
        return false;
    }
    // Map new page table in itself.
    Memory::map(newPageTable, newPageTable, newPageTable
                , (u64)Memory::PageTableFlag::Present
                | (u64)Memory::PageTableFlag::ReadWrite
                );

    Process* newProcess = new Process;

    // Copy each memory region's contents into newly allocated
    // memory.
    //u64 testVaddr = 0;
    //u64 expectedPaddr = 0;
    for (SinglyLinkedListNode<Memory::Region>* it = original->Memories.head(); it; it = it->next()) {
        Memory::Region& memory = it->value();
        Memory::Region newMemory{memory};
        u64 newMemoryPages = u64(Memory::request_pages(memory.pages));
        if (!newMemoryPages) {
            // Out of memory.
            panic("OOM");
            while (true) {
                asm volatile ("hlt");
            }
        }
        newMemory.paddr = (void*)newMemoryPages;
        // Copy memory contents.
        memcpy(newMemory.paddr, memory.paddr, memory.length);
        // Map virtual addresses to new physical addresses.
        //for (u64 virtualAddress = (u64)newMemory.vaddr; virtualAddress < ((u64)newMemory.vaddr + newMemory.length); virtualAddress += PAGE_SIZE) {
        //    Memory::unmap(newPageTable, (void*)virtualAddress);
        //}
        // Map virtual addresses to new physical addresses.
        u64 virtualAddress = (u64)newMemory.vaddr;

        //testVaddr = virtualAddress;
        //expectedPaddr = newMemoryPages;

        for (u64 physicalAddress = newMemoryPages; physicalAddress < (newMemoryPages + newMemory.length); virtualAddress += PAGE_SIZE, physicalAddress += PAGE_SIZE) {
            Memory::map(newPageTable, (void*)virtualAddress, (void*)physicalAddress, memory.flags, Memory::ShowDebug::No);
        }

        //Memory::PageMapIndexer indexer((u64)testVaddr);
        //Memory::PageDirectoryEntry PDE;
        //PDE = newPageTable->entries[indexer.page_directory_pointer()];
        //auto* PDP = (Memory::PageTable*)((u64)PDE.address() << 12);
        //PDE = PDP->entries[indexer.page_directory()];
        //auto* PD = (Memory::PageTable*)((u64)PDE.address() << 12);
        //PDE = PD->entries[indexer.page_table()];
        //auto* PT = (Memory::PageTable*)((u64)PDE.address() << 12);
        //PDE = PT->entries[indexer.page()];
        //std::print("PHYS {:#016x} at VIRT {:#016x} == EXPECTING {:#016x}\n", PDE.address() << 12, testVaddr, expectedPaddr);

        // Add new memory region to new process.
        newProcess->add_memory_region(newMemory);
    }

    // Copy file descriptors.
    for (const auto& entry : original->FileDescriptors) {
        std::shared_ptr<FileMetadata> meta = SYSTEM->virtual_filesystem().file(entry);
        // Meta, isn't it? :p
        auto f = meta->device_driver()->open(meta->name());
        SYSTEM->virtual_filesystem().add_file(std::move(f), newProcess);
    }

    newProcess->State = Process::ProcessState::RUNNING;
    newProcess->CR3 = newPageTable;
    newProcess->CPU = original->CPU;
    newProcess->next_region_vaddr = original->next_region_vaddr;
    // Set child return value for `fork()`.
    newProcess->CPU.RAX = 0;

    pid_t cpid = Scheduler::add_process(newProcess);
    //pid_t cpid = 0;
    newProcess->ProcessID = cpid;
    return cpid;
}

void Scheduler::map_pages_in_all_processes(void* virtualAddress, void* physicalAddress, u64 mappingFlags, size_t pages, Memory::ShowDebug d) {
    for (SinglyLinkedListNode<Process*>* it = ProcessQueue->head(); it; it = it->next()) {
        Memory::map_pages(it->value()->CR3
                          , virtualAddress, physicalAddress
                          , mappingFlags
                          , pages
                          , d);
    }
}
