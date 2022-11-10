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

#ifndef LENSOR_OS_ELF_LOADER_H
#define LENSOR_OS_ELF_LOADER_H

#include "storage/device_drivers/dbgout.h"

#include <format>

#include <elf.h>
#include <file.h>
#include <integers.h>
#include <link_definitions.h>
#include <memory/common.h>
#include <memory/heap.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <smart_pointer.h>
#include <tss.h>
#include <virtual_filesystem.h>

// Uncomment the following directive for extra debug output.
#define DEBUG_ELF

#ifdef DEBUG_ELF
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...) void()
#endif

namespace ELF {
    /// Return zero when ELF header is of expected format.
    inline bool VerifyElf64Header(const Elf64_Ehdr& ElfHeader) {
#ifndef DEBUG_ELF
        if (ElfHeader.e_ident[EI_MAG0] != ELFMAG0
            || ElfHeader.e_ident[EI_MAG1] != ELFMAG1
            || ElfHeader.e_ident[EI_MAG2] != ELFMAG2
            || ElfHeader.e_ident[EI_MAG3] != ELFMAG3
            || ElfHeader.e_ident[EI_CLASS] != ELFCLASS64
            || ElfHeader.e_ident[EI_DATA] != ELFDATA2LSB
            || ElfHeader.e_type != ET_EXEC
            || ElfHeader.e_machine != EM_X86_64
            || ElfHeader.e_version != EV_CURRENT)
        {
            return false;
        }
        return true;
#else /* #ifndef DEBUG_ELF */
        if (ElfHeader.e_ident[EI_MAG0] != ELFMAG0
            || ElfHeader.e_ident[EI_MAG1] != ELFMAG1
            || ElfHeader.e_ident[EI_MAG2] != ELFMAG2
            || ElfHeader.e_ident[EI_MAG3] != ELFMAG3)
        {
            std::print("[ELF]: Invalid ELF64 header: Magic bytes incorrect.\r\n"
                       "  Bytes (given, expected):\r\n"
                       "    0: {:#02x}, {:#02x}\r\n"
                       "    1: {}, {}\r\n"
                       "    2: {}, {}\r\n"
                       "    3: {}, {}\r\n"
                       "\r\n"
                       , ElfHeader.e_ident[EI_MAG0], ELFMAG0
                       , ElfHeader.e_ident[EI_MAG1], ELFMAG1
                       , ElfHeader.e_ident[EI_MAG2], ELFMAG2
                       , ElfHeader.e_ident[EI_MAG3], ELFMAG3
                       );
            return false;
        }
        if (ElfHeader.e_ident[EI_CLASS] != ELFCLASS64) {
            std::print("[ELF]: Invalid ELF64 header: Incorrect class.\r\n");
            return false;
        }
        if (ElfHeader.e_ident[EI_DATA] != ELFDATA2LSB) {
            std::print("[ELF]: Invalid ELF64 header: Incorrect data type.\r\n");
            return false;
        }
        if (ElfHeader.e_type != ET_EXEC) {
            std::print("[ELF]: Invalid ELF64 header: Type is not executable.\r\n");
            return false;
        }
        if (ElfHeader.e_machine != EM_X86_64) {
            std::print("[ELF]: Invalid ELF64 header: Machine is not valid.\r\n");
            return false;
        }
        if (ElfHeader.e_version != EV_CURRENT) {
            std::print("[ELF]: Invalid ELF64 header: ELF version is not expected.\r\n");
            return false;
        }
        return true;
#endif /* #ifndef DEBUG_ELF */
    }

    inline bool CreateUserspaceElf64Process(VFS& vfs, ProcessFileDescriptor fd) {
        DBGMSG("Attempting to add userspace process from file descriptor {}\r\n", fd);
        Elf64_Ehdr elfHeader;
        bool read = vfs.read(fd, reinterpret_cast<u8*>(&elfHeader), sizeof(Elf64_Ehdr));
        if (read == false) {
            std::print("Failed to read ELF64 header.\r\n");
            return false;
        }
        if (VerifyElf64Header(elfHeader) == false) {
            std::print("Executable did not have valid ELF64 header.\r\n");
            return false;
        }

        // Copy current page table (fork)
        auto* newPageTable = Memory::clone_active_page_map();
        if (newPageTable == nullptr) {
            std::print("Failed to clone current page map for new process page map.\r\n");
            return false;
        }

        Memory::map(newPageTable, newPageTable, newPageTable
                    , (u64)Memory::PageTableFlag::Present
                    | (u64)Memory::PageTableFlag::ReadWrite
                    );

        size_t stack_flags = 0;
        stack_flags |= (size_t)Memory::PageTableFlag::Present;
        stack_flags |= (size_t)Memory::PageTableFlag::ReadWrite;
        stack_flags |= (size_t)Memory::PageTableFlag::UserSuper;

        // TODO: Keep track of allocated memory regions for process.
        // Load PT_LOAD program headers, mapping to vaddr as necessary.
        u64 programHeadersTableSize = elfHeader.e_phnum * elfHeader.e_phentsize;
        SmartPtr<Elf64_Phdr[]> programHeaders(new Elf64_Phdr[elfHeader.e_phnum], elfHeader.e_phnum);
        vfs.read(fd, (u8*)(programHeaders.get()), programHeadersTableSize, elfHeader.e_phoff);
        for (
             Elf64_Phdr* phdr = programHeaders.get();
             (u64)phdr < (u64)programHeaders.get() + programHeadersTableSize;
             phdr++)
        {

            DBGMSG("Program header: type={}, offset={}\r\n"
                   "  filesz={:#016x}, memsz={:#016x}\r\n"
                   , phdr->p_type
                   , phdr->p_offset
                   , phdr->p_filesz
                   , phdr->p_memsz
                   );

            if (phdr->p_type == PT_LOAD) {
                // Allocate pages for program.
                u64 pages = (phdr->p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
                // Should I just use the kernel heap for this? It could grow very large...
                u8* loadedProgram = reinterpret_cast<u8*>(Memory::request_pages(pages));
                memset(loadedProgram, 0, phdr->p_memsz);
                bool read = vfs.read(fd, loadedProgram, phdr->p_filesz, phdr->p_offset);
                if (read == false)
                    return false;

                DBGMSG("[ELF]: Loaded program header ({} bytes) from file {} at byte offset {}\r\n"
                       , phdr->p_filesz
                       , fd
                       , phdr->p_offset
                       );

                // Virtually map allocated pages.
                u64 virtAddress = phdr->p_vaddr;
                size_t flags = 0;
                flags |= (size_t)Memory::PageTableFlag::Present;
                flags |= (size_t)Memory::PageTableFlag::UserSuper;
                if (phdr->p_flags & PF_W) {
                    flags |= (size_t)Memory::PageTableFlag::ReadWrite;
                }
                if (!(phdr->p_flags & PF_X)) {
                    flags |= (size_t)Memory::PageTableFlag::NX;
                }
                for (u64 t = 0; t < pages * PAGE_SIZE; t += PAGE_SIZE) {
                    Memory::map(newPageTable
                                , (void*)(virtAddress + t)
                                , loadedProgram + t
                                , flags
                                , Memory::ShowDebug::No
                                );
                }
            }
            else if (phdr->p_type == PT_GNU_STACK) {
                DBGMSG("[ELF]: Stack permissions set by GNU_STACK program header.\r\n");
                if (!(phdr->p_flags & PF_X)){
                    stack_flags |= (size_t)Memory::PageTableFlag::NX;}
            }
        }
        auto* process = new Process{};
        if (process == nullptr) {
            std::print("[ELF]: Couldn't allocate process structure for new userspace process\r\n");
            return false;
        }
        constexpr u64 UserProcessStackSizePages = 4;
        constexpr u64 UserProcessStackSize = UserProcessStackSizePages * PAGE_SIZE;
        u64 newStackBottom = (u64)Memory::request_pages(UserProcessStackSizePages);
        if (newStackBottom == 0) {
            std::print("[ELF]: Couldn't allocate stack for new userspace process\r\n");
            return false;
        }
        u64 newStackTop = newStackBottom + UserProcessStackSize;
        for (u64 t = newStackBottom; t < newStackTop; t += PAGE_SIZE)
            map(newPageTable, (void*)t, (void*)t, stack_flags);

        // Keep track of stack, as it is a memory region that remains
        // for the duration of the process, and should only be freed
        // when it exits.
        process->add_memory_region((void*)newStackBottom,
                                   (void*)newStackBottom,
                                   UserProcessStackSize);

        // Open stdin, stdout, and stderr.
        auto meta = new FileMetadata(String("stdout")
                                     , false
                                     , vfs.StdoutDriver.get()
                                     , nullptr
                                     , 0, 0);

        auto file = std::make_shared<OpenFileDescription>(vfs.StdoutDriver.get(), *meta);
        vfs.add_file(file, process);
        vfs.add_file(file, process);
        vfs.add_file(std::move(file), process);
        vfs.print_debug();

        std::print("[ELF] ProcFds:\r\n");
        u64 n = 0;
        for (const auto& entry : process->FileDescriptors) {
            std::print("  {} -> {}\r\n", n, entry);
            n++;
        }

        // New page map.
        process->CR3 = newPageTable;
        // New stack.
        process->CPU.RBP = newStackTop;
        process->CPU.RSP = newStackTop;
        process->CPU.Frame.sp = newStackTop;
        // Entry point.
        process->CPU.Frame.ip = elfHeader.e_entry;
        // Ring 3 GDT segment selectors.
        process->CPU.Frame.cs = 0x18 | 0b11;
        process->CPU.Frame.ss = 0x20 | 0b11;
        // Enable interrupts after jump.
        process->CPU.Frame.flags = 0b1010000010;
        Scheduler::add_process(process);
        return true;
    }
}

#endif /* LENSOR_OS_ELF_LOADER_H */
