#ifndef LENSOR_OS_ELF_LOADER_H
#define LENSOR_OS_ELF_LOADER_H

#include <smart_pointer.h>
#include <integers.h>
#include <elf.h>
#include <file.h>
#include <memory/common.h>
#include <memory/heap.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <tss.h>
#include <virtual_filesystem.h>

// Uncomment the following directive for extra debug output.
//#define DEBUG_ELF

namespace ELF {
    /// Return zero when ELF header is of expected format.
    bool VerifyElf64Header(const Elf64_Ehdr& ElfHeader) {
        if (ElfHeader.e_ident[EI_MAG0] != ELFMAG0
            || ElfHeader.e_ident[EI_MAG1] != *ELFMAG1
            || ElfHeader.e_ident[EI_MAG2] != *ELFMAG2
            || ElfHeader.e_ident[EI_MAG3] != *ELFMAG3
            || ElfHeader.e_ident[EI_CLASS] != ELFCLASS64
            || ElfHeader.e_ident[EI_DATA] != ELFDATA2LSB
            || ElfHeader.e_type != ET_EXEC
            || ElfHeader.e_machine != EM_X86_64
            || ElfHeader.e_version != EV_CURRENT)
        {
            return false;
        }
        return true;
    }

    bool RunUserspaceElf64(VFS& vfs, FileDescriptor fd) {
        Elf64_Ehdr elfHeader;
        vfs.read(fd, reinterpret_cast<u8*>(&elfHeader), sizeof(Elf64_Ehdr));
        if (VerifyElf64Header(elfHeader) == false)
            return false;

        // Copy current page table (fork)
        Memory::PageDirectoryEntry PDE;
        Memory::PageTable* oldPageTable = Memory::active_page_map();
        auto* newPageTable = reinterpret_cast<Memory::PageTable*>(Memory::request_page());
        if (newPageTable == nullptr) {
            dbgmsg_s("Failed to allocate memory for new process page map level four.\r\n");
            return false;
        }
        memset(newPageTable, 0, PAGE_SIZE);
        for (u64 i = 0; i < 512; ++i) {
            PDE = oldPageTable->entries[i];
            if (PDE.flag(Memory::PageTableFlag::Present) == false)
                continue;

            auto* newPDP = (Memory::PageTable*)Memory::request_page();
            if (newPDP == nullptr) {
                dbgmsg_s("Failed to allocate memory for new process page directory pointer table.\r\n");
                return false;
            }
            auto* oldTable = (Memory::PageTable*)((u64)PDE.address() << 12);
            for (u64 j = 0; j < 512; ++j) {
                PDE = oldTable->entries[j];
                if (PDE.flag(Memory::PageTableFlag::Present) == false)
                    continue;

                auto* newPD = (Memory::PageTable*)Memory::request_page();
                if (newPD == nullptr) {
                    dbgmsg_s("Failed to allocate memory for new process page directory table.\r\n");
                    return false;
                }
                auto* oldPD = (Memory::PageTable*)((u64)PDE.address() << 12);
                for (u64 k = 0; k < 512; ++k) {
                    PDE = oldPD->entries[k];
                    if (PDE.flag(Memory::PageTableFlag::Present) == false)
                        continue;

                    auto* newPT = (Memory::PageTable*)Memory::request_page();
                    if (newPT == nullptr) {
                        dbgmsg_s("Failed to allocate memory for new process page table.\r\n");
                        return false;
                    }
                    auto* oldPT = (Memory::PageTable*)((u64)PDE.address() << 12);
                    memcpy(oldPT, newPT, PAGE_SIZE);

                    PDE = oldPD->entries[k];
                    PDE.set_address((u64)newPT >> 12);
                    newPD->entries[k] = PDE;
                }
                PDE = oldTable->entries[j];
                PDE.set_address((u64)newPD >> 12);
                newPDP->entries[j] = PDE;
            }
            PDE = oldPageTable->entries[i];
            PDE.set_address((u64)newPDP >> 12);
            newPageTable->entries[i] = PDE;
        }

        for (u64 t = 0; t < Memory::total_ram(); t += PAGE_SIZE)
            unmap(newPageTable, (void*)t);

        Memory::map(newPageTable, newPageTable, newPageTable, true);
        u64 programHeadersTableSize = elfHeader.e_phnum * elfHeader.e_phentsize;
        SmartPtr<Elf64_Phdr[]> programHeaders(new Elf64_Phdr[elfHeader.e_phnum], elfHeader.e_phnum);
        vfs.read(fd, (u8*)(programHeaders.get()), programHeadersTableSize, elfHeader.e_phoff);
        for (
             Elf64_Phdr* phdr = programHeaders.get();
             (u64)phdr < (u64)programHeaders.get() + programHeadersTableSize;
             phdr++)
        {
#ifdef DEBUG_ELF
            dbgmsg("Program header: type=%ul, offset=%ull\r\n"
                   , phdr->p_type
                   , phdr->p_offset
                   );
#endif /* #ifdef DEBUG_ELF */
            if (phdr->p_type == PT_LOAD) {
                // Allocate pages for program.
                u64 pages = (phdr->p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;

                // Should I just use the kernel heap for this? It would grow very large...
                u8* loadedProgram = reinterpret_cast<u8*>(Memory::request_pages(pages));
                memset(loadedProgram, 0, phdr->p_memsz);
                bool read = vfs.read(fd, loadedProgram, phdr->p_filesz, phdr->p_offset);
                if (read == false)
                    return false;

#ifdef DEBUG_ELF
                dbgmsg("[ELF]: Loaded program header (%ull bytes) from file %ull at byte offset %ull\r\n"
                       , phdr->p_filesz
                       , fd
                       , phdr->p_offset
                       );
#endif /* #ifdef DEBUG_ELF */
                // Virtually map allocated pages.
                u64 virtAddress = phdr->p_vaddr;
                for (u64 t = 0; t < pages * PAGE_SIZE; t += PAGE_SIZE) {
                    Memory::map(newPageTable
                                , (void*)(virtAddress + t)
                                , loadedProgram
                                , (1 << Memory::PageTableFlag::Present)
                                | (1 << Memory::PageTableFlag::ReadWrite)
                                | (1 << Memory::PageTableFlag::UserSuper)
                                , Memory::ShowDebug::Yes
                                );
                }
            }
        }



        // TODO: Create new scheduler process.
        Process* process = new Process;
        if (process == nullptr)
            return false;

        process->CR3 = newPageTable;

        Memory::flush_page_map(newPageTable);
        dbgmsg("Jumping to userland Elf64 at %x\r\n", elfHeader.e_entry);
        jump_to_userland_function((void*)elfHeader.e_entry);
  
        return true;
    }
}

#endif /* LENSOR_OS_ELF_LOADER_H */