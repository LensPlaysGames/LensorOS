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

#include <format>
#include <string>

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
#include <system.h>
#include <tss.h>
#include <virtual_filesystem.h>

// Uncomment the following directive for extra debug output.
//#define DEBUG_ELF

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
            std::print("[ELF]: Invalid ELF64 header: Magic bytes incorrect.\n"
                       "  Bytes (given, expected):\n"
                       "    0: {:#02x}, {:#02x}\n"
                       "    1: {}, {}\n"
                       "    2: {}, {}\n"
                       "    3: {}, {}\n"
                       "\n"
                       , ElfHeader.e_ident[EI_MAG0], ELFMAG0
                       , ElfHeader.e_ident[EI_MAG1], ELFMAG1
                       , ElfHeader.e_ident[EI_MAG2], ELFMAG2
                       , ElfHeader.e_ident[EI_MAG3], ELFMAG3
                       );
            return false;
        }
        if (ElfHeader.e_ident[EI_CLASS] != ELFCLASS64) {
            std::print("[ELF]: Invalid ELF64 header: Incorrect class.\n");
            return false;
        }
        if (ElfHeader.e_ident[EI_DATA] != ELFDATA2LSB) {
            std::print("[ELF]: Invalid ELF64 header: Incorrect data type.\n");
            return false;
        }
        if (ElfHeader.e_type != ET_EXEC) {
            std::print("[ELF]: Invalid ELF64 header: Type is not executable.\n");
            return false;
        }
        if (ElfHeader.e_machine != EM_X86_64) {
            std::print("[ELF]: Invalid ELF64 header: Machine is not valid.\n");
            return false;
        }
        if (ElfHeader.e_version != EV_CURRENT) {
            std::print("[ELF]: Invalid ELF64 header: ELF version is not expected.\n");
            return false;
        }
        return true;
#endif /* #ifndef DEBUG_ELF */
    }

    inline bool LoadUserspaceElf64Process(Process* process, Memory::PageTable* pageTable,
                                          ProcessFileDescriptor fd, const Elf64_Ehdr& elfHeader,
                                          const std::vector<std::string_view>& args = {},
                                          const std::vector<std::string_view>& env = {}) {
        VFS& vfs = SYSTEM->virtual_filesystem();

        size_t stack_flags = 0;
        stack_flags |= (size_t)Memory::PageTableFlag::Present;
        stack_flags |= (size_t)Memory::PageTableFlag::ReadWrite;
        stack_flags |= (size_t)Memory::PageTableFlag::UserSuper;

        // Load PT_LOAD program headers, mapping to vaddr as necessary.
        u64 programHeadersTableSize = elfHeader.e_phnum * elfHeader.e_phentsize;
        std::vector<Elf64_Phdr> programHeaders(elfHeader.e_phnum);
        vfs.read(fd, (u8*)(programHeaders.data()), programHeadersTableSize, elfHeader.e_phoff);
        for (
             Elf64_Phdr* phdr = programHeaders.data();
             (u64)phdr < (u64)programHeaders.data() + programHeadersTableSize;
             phdr++)
        {

            DBGMSG("Program header: type={}, offset={}\n"
                   "  filesz={:#016x}, memsz={:#016x}\n"
                   , phdr->p_type
                   , phdr->p_offset
                   , phdr->p_filesz
                   , phdr->p_memsz
                   );

            /// Warn if the size is zero.
            if (phdr->p_memsz == 0) std::print("[ELF]: Warning: program header has zero size.\n");
            if (phdr->p_type == PT_LOAD) {
                // The program header may not be aligned to a page boundary, in which
                // case we need to take the offset and round *down* to the nearest page.
                u64 size_to_load = phdr->p_memsz + phdr->p_vaddr % PAGE_SIZE;

                // Allocate pages for program.
                u64 pages = (size_to_load + PAGE_SIZE - 1) / PAGE_SIZE;

                // Should I just use the kernel heap for this? It could grow very large...
                // Zero out the allocated memory.
                u8* loadedProgram = reinterpret_cast<u8*>(Memory::request_pages(pages));
                memset(loadedProgram, 0, size_to_load);

                // Read the program into memory. If the program header does not start
                // at a page boundary, then we need to offset the read by the offset
                // into the page.
                u64 offset = phdr->p_vaddr % PAGE_SIZE;
                auto n_read = vfs.read(fd, loadedProgram + offset, phdr->p_filesz, phdr->p_offset);
                if (n_read < 0 || size_t(n_read) != phdr->p_filesz) {
                    std::print("[ELF] Could not read program data from file {}\n" , fd);
                    return false;
                }

                DBGMSG("[ELF]: Loaded program header ({} bytes) from file {} at byte offset {}\n"
                       , phdr->p_filesz
                       , fd
                       , phdr->p_offset
                       );

                // Virtually map allocated pages.
                size_t flags = 0;
                flags |= (size_t)Memory::PageTableFlag::Present;
                flags |= (size_t)Memory::PageTableFlag::UserSuper;
                if (phdr->p_flags & PF_W) {
                    flags |= (size_t)Memory::PageTableFlag::ReadWrite;
                }
                if (!(phdr->p_flags & PF_X)) {
                    flags |= (size_t)Memory::PageTableFlag::NX;
                }
                u64 virtAddress = phdr->p_vaddr;
                for (u64 t = 0; t < pages * PAGE_SIZE; t += PAGE_SIZE) {
                    Memory::map(pageTable
                                , (void*)(virtAddress + t)
                                , loadedProgram + t
                                , flags
                                , Memory::ShowDebug::No
                                );
                }
                process->add_memory_region((void*)phdr->p_vaddr
                                           , (void*)loadedProgram
                                           , pages * PAGE_SIZE
                                           , flags
                                           );
            }
            else if (phdr->p_type == PT_GNU_STACK) {
                DBGMSG("[ELF]: Stack permissions set by GNU_STACK program header.\n");
                if (!(phdr->p_flags & PF_X)){
                    stack_flags |= (size_t)Memory::PageTableFlag::NX;}
            }
        }

        /// TODO: `new` should *never* return nullptr. This check shouldn’t be necessary.
        if (process == nullptr) {
            std::print("[ELF]: Couldn't allocate process structure for new userspace process\n");
            return false;
        }
        constexpr u64 UserProcessStackSizePages = 4;
        constexpr u64 UserProcessStackSize = UserProcessStackSizePages * PAGE_SIZE;
        // FIXME: There's no reason this memory needs to be physically contiguous.
        u64 newStackBottom = (u64)Memory::request_pages(UserProcessStackSizePages);
        if (newStackBottom == 0) {
            std::print("[ELF]: Couldn't allocate stack for new userspace process\n");
            return false;
        }
        u64 stack_top_address = newStackBottom + UserProcessStackSize;
        Memory::map_pages(pageTable, (void*)newStackBottom, (void*)newStackBottom, stack_flags, UserProcessStackSizePages, Memory::ShowDebug::No);

        // Keep track of stack, as it is a memory region that remains
        // for the duration of the process, and should only be freed
        // when it exits.
        process->add_memory_region((void*)newStackBottom,
                                   (void*)newStackBottom,
                                   UserProcessStackSize,
                                   stack_flags);

        // TODO: Max argument length? Maximum environment length?

        // Copy environment contents to the stack, keeping track of addresses.
        std::vector<usz> envp_addresses;
        for (auto str : env) {
            usz size = str.size() + 1;
            size += 16 - (size & 15);
            stack_top_address -= size;
            envp_addresses.push_back(stack_top_address);
            memcpy(reinterpret_cast<void*>(stack_top_address), str.data(), str.size());
            reinterpret_cast<char*>(stack_top_address)[str.size()] = 0;
        }

        // Copy arguments contents to the stack, keeping track of addresses.
        std::vector<u64> argv_addresses;
        for (auto str : args) {
            usz size = str.size() + 1;
            size += 16 - (size & 15);
            stack_top_address -= size;
            argv_addresses.push_back(stack_top_address);
            memcpy(reinterpret_cast<void*>(stack_top_address), str.data(), str.size());
            reinterpret_cast<char*>(stack_top_address)[str.size()] = 0;
        }

        // Align stack to 16 if it will be misaligned by pushing the
        // addresses on to the stack.
        // NOTE: `+ 1`s to account for NULL terminators of envp and argv, as well as argc.
        if ((envp_addresses.size() + 1 + argv_addresses.size() + 1 + 1) % 2 != 0) {
            stack_top_address -= 8;
            *reinterpret_cast<u64*>(stack_top_address) = 0;
        }

        // Write null pointer to end of envp.
        stack_top_address -= sizeof(char*);
        *reinterpret_cast<char**>(stack_top_address) = nullptr;

        // Write envp addresses to the stack.
        for (auto it = envp_addresses.rbegin(); it != envp_addresses.rend(); ++it) {
            stack_top_address -= sizeof(char*);
            *reinterpret_cast<u64*>(stack_top_address) = *it;
        }

        // Write null pointer to end of argv.
        stack_top_address -= sizeof(char*);
        *reinterpret_cast<char**>(stack_top_address) = nullptr;

        // Write argv addresses to the stack.
        for (auto it = argv_addresses.rbegin(); it != argv_addresses.rend(); ++it) {
            stack_top_address -= sizeof(char*);
            *reinterpret_cast<u64*>(stack_top_address) = *it;
        }

        // Write argc to the stack.
        //
        // Even though argc is an int in C, the ABI requires that it be
        // pushed as a 64-bit value.
        stack_top_address -= sizeof(size_t);
        *reinterpret_cast<size_t*>(stack_top_address) = args.size();

#ifdef DEBUG_ELF
        auto _argc = *reinterpret_cast<int*>(stack_top_address);
        auto _argv = reinterpret_cast<char**>(stack_top_address + sizeof(size_t));

        std::print("[ELF] Program Arguments \n"
                   "          stack: {:#016x}\n"
                   "          argc: {}\n"
                   "          argv: {}\n"
                   , stack_top_address
                   , _argc
                   , (void*)_argv);

        for (int i = 0; i < _argc; ++i) {
            std::print("argv[{}]: {}\n", i, _argv[i]);
        }
#endif

        if (stack_top_address % 16 != 0) {
            if (stack_top_address % 8 != 0) {
                panic("STACK UNALIGNED\n");
            } else {
                panic("STACK 8-BYTE ALIGNED\n");
            }
        }

        // TODO: Abstract x86_64 specific stuff, somehow.

        // New stack.
        process->CPU.RBP = (u64)(stack_top_address);
        process->CPU.RSP = (u64)(stack_top_address);
        process->CPU.Frame.sp = (u64)(stack_top_address);
        // Entry point.
        process->CPU.Frame.ip = elfHeader.e_entry;
        // Ring 3 GDT segment selectors.
        process->CPU.Frame.cs = 0x18 | 3;
        process->CPU.Frame.ss = 0x20 | 3;
        // Enable interrupts after jump.
        process->CPU.Frame.flags = 0b1010000010;

#ifdef DEBUG_ELF
        Scheduler::print_debug();
#endif

        return true;
    }

    inline bool ReplaceUserspaceElf64Process(Process* process, ProcessFileDescriptor fd, const std::vector<std::string_view>& args = {}) {
        VFS& vfs = SYSTEM->virtual_filesystem();
        DBGMSG("Attempting to add userspace process from file descriptor {}\n", fd);
        Elf64_Ehdr elfHeader;
        bool read = vfs.read(fd, reinterpret_cast<u8*>(&elfHeader), sizeof(Elf64_Ehdr));
        if (read == false) {
            std::print("Failed to read ELF64 header.\n");
            return false;
        }
        if (VerifyElf64Header(elfHeader) == false) {
            std::print("Executable did not have valid ELF64 header.\n");
            return false;
        }

        // Unmap and free old process memory, if header is valid and things look good to go.
        for (const auto& region : process->Memories) {
            Memory::unmap_pages(process->CR3, region.vaddr, region.pages, Memory::ShowDebug::No);
            Memory::free_pages(region.paddr, region.pages);
        }
        // Clear memories list.
        while (process->Memories.remove(0));

        return LoadUserspaceElf64Process(process, process->CR3, fd, elfHeader, args);
    }

/// @param args  Should NEVER be empty, as argv[0] should ALWAYS contain executable invocation (filepath).
    inline bool CreateUserspaceElf64Process(ProcessFileDescriptor fd, const std::vector<std::string_view>& args) {
        if (!args.size()) {
            std::print("Can not invoke process with zero arguments: at least invocation (argv[0]) is required\n");
            return false;
        }
        VFS& vfs = SYSTEM->virtual_filesystem();
        DBGMSG("Attempting to add userspace process from file descriptor {}\n", fd);
        Elf64_Ehdr elfHeader;
        bool read = vfs.read(fd, reinterpret_cast<u8*>(&elfHeader), sizeof(Elf64_Ehdr));
        if (read == false) {
            std::print("Failed to read ELF64 header.\n");
            return false;
        }
        if (VerifyElf64Header(elfHeader) == false) {
            std::print("Executable did not have valid ELF64 header.\n");
            return false;
        }

        auto* process = new Process{};
        process->State = Process::ProcessState::SLEEPING;
        pid_t pid = Scheduler::add_process(process);

        // Copy current page table (fork)
        auto* newPageTable = Memory::clone_active_page_map();
        if (newPageTable == nullptr) {
            std::print("Failed to clone current page map for new process page map.\n");
            Scheduler::remove_process(pid, -1);
            return false;
        }
        process->CR3 = newPageTable;

        Memory::map(newPageTable, newPageTable, newPageTable
                    , (u64)Memory::PageTableFlag::Present
                    | (u64)Memory::PageTableFlag::ReadWrite
                    );

        if (!LoadUserspaceElf64Process(process, newPageTable, fd, elfHeader, args)) {
            // Remove process from process list
            Scheduler::remove_process(pid, -1);
            return false;
        }

        // Open stdin.
        vfs.add_file(vfs.StdinDriver->open("stdin"), process);
        // Open stdout and stderr
        auto outmeta = std::make_shared<FileMetadata>("stdout", sdd(vfs.StdoutDriver), 0, nullptr);
        vfs.add_file(outmeta, process);
        vfs.add_file(std::move(outmeta), process);
        vfs.print_debug();

#ifdef DEBUG_ELF
        std::print("[ELF] ProcFds:\n");
        u64 n = 0;
        for (const auto& entry : process->FileDescriptors) {
            std::print("  {} -> {}\n", n, entry);
            n++;
        }
#endif

        // New page map.
        process->CR3 = newPageTable;

        process->ExecutablePath = args[0];
        process->WorkingDirectory =
            process->ExecutablePath.substr(0, process->ExecutablePath.find_last_of("/"));

        // Make scheduler aware that this process may be run.
        process->State = Process::ProcessState::RUNNING;
        return true;
    }
}

#undef DBGMSG

#endif /* LENSOR_OS_ELF_LOADER_H */
