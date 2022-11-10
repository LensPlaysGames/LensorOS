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
#include <vfs_forward.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_SYSCALLS

#ifdef DEBUG_SYSCALLS
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...)
#endif

/// SYSCALL NAMING SCHEME:
/// "sys$" + number + "_" + descriptive name

[[maybe_unused]]
constexpr const char* sys$_dbgfmt = "[SYS$]: {} -- {}\r\n";

ProcessFileDescriptor sys$0_open(const char* path) {
    DBGMSG(sys$_dbgfmt, 0, "open");
    return static_cast<ProcessFileDescriptor>(SYSTEM->virtual_filesystem().open(path).Process);
}

void sys$1_close(ProcessFileDescriptor fd) {
    DBGMSG(sys$_dbgfmt, 1, "close");
    SYSTEM->virtual_filesystem().close(static_cast<ProcessFileDescriptor>(fd));
}

// TODO: This should return the amount of bytes read.
int sys$2_read(ProcessFileDescriptor fd, u8* buffer, u64 byteCount) {
    DBGMSG(sys$_dbgfmt, 2, "read");
    DBGMSG("  file descriptor: {}\r\n"
           "  buffer address:  {}\r\n"
           "  byte count:      {}\r\n"
           "\r\n"
           , fd
           , (void*) buffer
           , byteCount
           );
    return SYSTEM->virtual_filesystem().read(static_cast<ProcessFileDescriptor>(fd), buffer, byteCount, 0);
}

// TODO: This should return the amount of bytes written.
int sys$3_write(ProcessFileDescriptor fd, u8* buffer, u64 byteCount) {
    DBGMSG(sys$_dbgfmt, 3, "write");
    DBGMSG("  file descriptor: {}\r\n"
           "  buffer address:  {}\r\n"
           "  byte count:      {}\r\n"
           "\r\n"
           , fd
           , (void*) buffer
           , byteCount
           );
    return SYSTEM->virtual_filesystem().write(static_cast<ProcessFileDescriptor>(fd), buffer, byteCount, 0);
}

void sys$4_poke() {
    DBGMSG(sys$_dbgfmt, 4, "poke");
    DBGMSG("Poke from userland!\r\n");
}

void sys$5_exit([[maybe_unused]] int status) {
    DBGMSG(sys$_dbgfmt, 5, "exit");
    DBGMSG("  status: {}\r\n"
           "\r\n"
           , status
           );
    pid_t pid = Scheduler::CurrentProcess->value()->ProcessID;
    Scheduler::remove_process(pid);
    DBGMSG("[SYS$]: exit({}) -- Removed process {}\r\n", status, pid);
}

void* sys$6_map(void* address, usz size, u64 flags) {
    DBGMSG(sys$_dbgfmt, 6, "map");
    DBGMSG("  address: {}\r\n"
           "  size:    {}\r\n" // TODO: %ull is wrong, we need a size type format
           "  flags:   {}\r\n"
           "\r\n"
           , address
           , size
           , flags
           );

    Process* process = Scheduler::CurrentProcess->value();

    usz pages = 1;
    pages += size / PAGE_SIZE;

    // Allocate physical RAM
    void* paddr = Memory::request_pages(pages);

    // If address is NULL, pick an address to place memory at.
    if (!address) {
        address = (void*)process->next_region_vaddr;
        process->next_region_vaddr += pages * PAGE_SIZE;
    }

    // Add memory region to current process
    process->add_memory_region(address, paddr, size);

    // TODO: Convert given flags to Memory::PageTableFlag
    // TODO: Figure out what flags we are given (libc, ig).

    // Map virtual address to physical with proper flags
    // Don't forget to map all pages!
    usz vaddr_it = (usz)address;
    usz paddr_it = (usz)paddr;
    for (usz i = 0; i < pages; ++i) {
        Memory::map(process->CR3, (void*)vaddr_it, (void*)paddr_it, flags);
        vaddr_it = vaddr_it + PAGE_SIZE;
        paddr_it = paddr_it + PAGE_SIZE;
    }

    // Return usable address.
    return address;
}

void sys$7_unmap(void* address) {
    DBGMSG(sys$_dbgfmt, 7, "unmap");
    DBGMSG("  address: {}\r\n"
           "\r\n"
           , address
           );

    Process* process = Scheduler::CurrentProcess->value();

    // Search current process' memories for matching address.
    auto* region = process->Memories.head();
    for (; region; region = region->next()) {
        if (region->value().vaddr == address) {
            break;
        }
    }

    // Ignore an attempt to unmap invalid address.
    // TODO: If a single program is freeing invalid addresses over and
    // over, it's a good sign they are a bad actor and should maybe
    // just be stopped.
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
    process->remove_memory_region(address);
    return;
}

void sys$8_time(Time::tm* time) {
    DBGMSG(sys$_dbgfmt, 8, "time");
    DBGMSG("  tm: {}\r\n"
           "\r\n"
           , (void*) time
           );
    if (!time) { return; }
    Time::fill_tm(time);
}


/// Wait for process with PID to terminate. If process with PID is
/// invalid, return immediately.
void sys$9_waitpid(pid_t pid) {
    (void)pid;
    // TODO: Return immediately if PID isn't valid.
    // TODO: Figure out how to wait for something without taking up
    // precious processor time just spinning here.
}

/// Copy the current process, resuming execution in both just after the
/// fork call, but with the return value to each different (child gets
/// zero, parent gets child's PID).
void sys$10_fork() {
    // TODO: Copy current process
    // TODO: Set return value of each process (this one and the new
    // child).
}

/// Replace the current process with a new process, specified by an
/// executable found at PATH.
void sys$11_exec(char *path) {
    (void)path;
    // TODO: Ensure valid arguments
    // TODO: Replace current process with new process, if successfully
    // loaded...
}

/// The second file descriptor given will be associated with the file
/// description of the first.
void sys$12_repfd(ProcessFileDescriptor fd, ProcessFileDescriptor replaced) {
    (void)fd;
    (void)replaced;
    // TODO: Point REPLACED to same file description as FD
}

/// Create two file descriptors. One of which can be read from, and the
/// other which can be written to.
ProcessFileDescriptor* sys$13_pipe() {
    // TODO: Allocate process file descriptors array somewhere that
    // makes sense.
    // TODO: Populate fds with file descriptors that will act
    // appropriately; basically, either conect each to a real file, or
    // somehow manage buffers for it.
    return nullptr;
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

    // MORE PROCESSES & SCHEDULING
    (void*)sys$9_waitpid,
    (void*)sys$10_fork,
    (void*)sys$11_exec,
    (void*)sys$12_repfd,
    (void*)sys$13_pipe,
};
