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

#include <interrupts/syscalls.h>

#include <debug.h>
#include <file.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/region.h>
#include <memory/virtual_memory_manager.h>
#include <rtc.h>
#include <scheduler.h>
#include <system.h>
#include <time.h>
#include <virtual_filesystem.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_SYSCALLS

/// SYSCALL NAMING SCHEME:
/// "sys$" + number + "_" + descriptive name

constexpr const char* sys$_dbgfmt = "[SYS$]: %d -- %s\r\n";

FileDescriptor sys$0_open(const char* path) {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 0, "open");
#endif /* #ifdef DEBUG_SYSCALLS */
    return static_cast<FileDescriptor>(SYSTEM->virtual_filesystem().open(path).Process);
}

void sys$1_close(FileDescriptor fd) {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 1, "close");
#endif /* #ifdef DEBUG_SYSCALLS */
    SYSTEM->virtual_filesystem().close(static_cast<ProcessFileDescriptor>(fd));
}

// TODO: This should return the amount of bytes read.
int sys$2_read(FileDescriptor fd, u8* buffer, u64 byteCount) {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 2, "read");
    dbgmsg("  file descriptor: %d\r\n"
           "  buffer address:  %x\r\n"
           "  byte count:      %ull\r\n"
           "\r\n"
           , fd
           , buffer
           , byteCount
           );
#endif /* #ifdef DEBUG_SYSCALLS */
    return SYSTEM->virtual_filesystem().read(static_cast<ProcessFileDescriptor>(fd), buffer, byteCount, 0);
}

// TODO: This should return the amount of bytes written.
int sys$3_write(FileDescriptor fd, u8* buffer, u64 byteCount) {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 3, "write");
    dbgmsg("  file descriptor: %d\r\n"
           "  buffer address:  %x\r\n"
           "  byte count:      %ull\r\n"
           "\r\n"
           , fd
           , buffer
           , byteCount
           );
#endif /* #ifdef DEBUG_SYSCALLS */
    return SYSTEM->virtual_filesystem().write(static_cast<ProcessFileDescriptor>(fd), buffer, byteCount, 0);
}

void sys$4_poke() {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 4, "poke");
#endif /* #ifdef DEBUG_SYSCALLS */
    // Prevent unused warning
    (void)sys$_dbgfmt;
    dbgmsg_s("Poke from userland!\r\n");
}

void sys$5_exit(int status) {
    pid_t pid = Scheduler::CurrentProcess->value()->ProcessID;
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 5, "exit");
    dbgmsg("  status: %i\r\n"
           "\r\n"
           , status
           );
#endif /* #ifdef DEBUG_SYSCALLS */
    Scheduler::remove_process(pid);
    dbgmsg("[SYS$]: exit(%i) -- Removed process %ull\r\n", status, pid);
}

void* sys$6_map(void* address, usz size, u64 flags) {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 6, "map");
    dbgmsg("  address: %p\r\n"
           "  size:    %ul\r\n" // TODO: %ul is wrong, we need a size type format
           "  flags:   %ull\r\n"
           "\r\n"
           , address
           , size
           , flags
           );
#endif /* #ifdef DEBUG_SYSCALLS */

    usz pages = 1;
    pages += size / PAGE_SIZE;

    // Allocate physical RAM
    void* paddr = Memory::request_pages(pages);

    // If address is NULL, pick an address to place memory at.
    if (!address) {
        address = (void*)Scheduler::CurrentProcess->value()->next_region_vaddr;
        Scheduler::CurrentProcess->value()->next_region_vaddr += pages * PAGE_SIZE;
    }

    // Add memory region to current process
    Scheduler::CurrentProcess->value()->add_memory_region(address, paddr, size);

    // TODO: Convert given flags to Memory::PageTableFlag
    // TODO: Figure out what flags we are given (libc, ig).

    // Map virtual address to physical with proper flags
    // Don't forget to map all pages!
    usz vaddr_it = (usz)address;
    usz paddr_it = (usz)paddr;
    for (usz i = 0; i < pages; ++i) {
        Memory::map((void*)vaddr_it, (void*)paddr_it, flags);
        vaddr_it = vaddr_it + PAGE_SIZE;
        paddr_it = paddr_it + PAGE_SIZE;
    }

    // Return usable address.
    return address;
}

void sys$7_unmap(void* address) {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 7, "unmap");
    dbgmsg("  address: %p\r\n"
           "\r\n"
           , address
           );
#endif /* #ifdef DEBUG_SYSCALLS */

    // Search current process' memories for matching address.
    SinglyLinkedListNode<Memory::Region>* region = Scheduler::CurrentProcess->value()->Memories.head();
    for (; region; region = region->next()) {
        if (region->value().vaddr == address) {
            break;
        }
    }

    // Ignore an attempt to unmap invalid address.
    if (!region) return;

    // Unmap memory from current process page table.
    // Don't forget to unmap all pages!
    usz vaddr_it = (usz)address;
    for (usz i = 0; i < region->value().length; ++i) {
        Memory::unmap((void*)vaddr_it);
        vaddr_it = vaddr_it + PAGE_SIZE;
    }

    // Free physical memory referred to by region.
    Memory::free_pages(region->value().paddr, region->value().pages);

    // Remove memory region from process memories list.
    Scheduler::CurrentProcess->value()->remove_memory_region(address);
    return;
}

void sys$8_time(Time::tm* time) {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 8, "time");
    dbgmsg("  tm: %p\r\n"
           "\r\n"
           , time
           );
#endif /* #ifdef DEBUG_SYSCALLS */

    if (!time) { return; }
    Time::fill_tm(time);
}

u64 num_syscalls = LENSOR_OS_NUM_SYSCALLS;
void* syscalls[LENSOR_OS_NUM_SYSCALLS] = {
    // FILE STUFFS
    (void*)sys$0_open,
    (void*)sys$1_close,
    (void*)sys$2_read,
    (void*)sys$3_write,
    (void*)sys$4_poke,

    // PROCESSES & SCHEDULING
    (void*)sys$5_exit,

    // MEMORY
    (void*)sys$6_map,
    (void*)sys$7_unmap,

    // MISCELLANEOUS
    (void*)sys$8_time,
};
