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

#include <algorithm>
#include <interrupts/syscalls.h>

#include <debug.h>
#include <elf_loader.h>
#include <file.h>
#include <linked_list.h>
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
#include <memory>

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
constexpr const char* sys$_dbgfmt = "[SYS$]: {} -- {}\n";

ProcessFileDescriptor sys$0_open(const char* path) {
    DBGMSG(sys$_dbgfmt, 0, "open");
    // FIXME: Validate path pointer.
    return SYSTEM->virtual_filesystem().open(path).Process;
}

void sys$1_close(ProcessFileDescriptor fd) {
    DBGMSG(sys$_dbgfmt, 1, "close");
    SYSTEM->virtual_filesystem().close(fd);
}

int sys$2_read(ProcessFileDescriptor fd, u8* buffer, u64 byteCount) {
    CPUState* cpu = nullptr;
    asm volatile ("mov %%r11, %0\n"
                  : "=r"(cpu)
                  );
    DBGMSG(sys$_dbgfmt, 2, "read");
    DBGMSG("  file descriptor: {}\n"
           "  buffer address:  {}\n"
           "  byte count:      {}\n"
           "\n"
           , fd
           , (void*) buffer
           , byteCount
           );
    // FIXME: Validate buffer pointer.

    // Save CPU state in case read blocks, aka calls yield.
    memcpy(&Scheduler::CurrentProcess->value()->CPU, cpu, sizeof(CPUState));
    return SYSTEM->virtual_filesystem().read(fd, buffer, byteCount, 0);
}

int sys$3_write(ProcessFileDescriptor fd, u8* buffer, u64 byteCount) {
    CPUState* cpu = nullptr;
    asm volatile ("mov %%r11, %0\n"
                  : "=r"(cpu)
                  );
    DBGMSG(sys$_dbgfmt, 3, "write");
    DBGMSG("  file descriptor: {}\n"
           "  buffer address:  {}\n"
           "  byte count:      {}\n"
           "\n"
           , fd
           , (void*) buffer
           , byteCount
           );
    // FIXME: Validate buffer pointer.

    // Save CPU state in case write blocks, aka calls yield.
    memcpy(&Scheduler::CurrentProcess->value()->CPU, cpu, sizeof(CPUState));
    return SYSTEM->virtual_filesystem().write(fd, buffer, byteCount, 0);
}

void sys$4_poke() {
    DBGMSG(sys$_dbgfmt, 4, "poke");
    std::print("Poke from userland!\n");
}

void sys$5_exit(int status) {
    DBGMSG(sys$_dbgfmt, 5, "exit");
    DBGMSG("  status: {}\n"
           "\n"
           , status
           );
    {
        pid_t pid = Scheduler::CurrentProcess->value()->ProcessID;
        bool success = Scheduler::remove_process(pid, status);
        if (!success)
            std::print("[EXIT]: Failure to remove process\n");
        else {
            DBGMSG("[SYS$]: exit({}) -- Removed process {}\n", status, pid);
        }
    }

    Scheduler::yield();
}

void* sys$6_map(void* address, usz size, u64 flags) {
    DBGMSG(sys$_dbgfmt, 6, "map");
    DBGMSG("  address: {}\n"
           "  size:    {}\n"
           "  flags:   {}\n"
           "\n"
           , address
           , size
           , flags
           );

    Process* process = Scheduler::CurrentProcess->value();

    usz pages = 1;
    pages += size / PAGE_SIZE;

    // Allocate physical RAM
    // TODO: There isn't really any reason these need to be contiguous.
    void* paddr = Memory::request_pages(pages);

    // If address is NULL, pick an address to place memory at.
    if (!address) {
        address = (void*)process->next_region_vaddr;
        process->next_region_vaddr += pages * PAGE_SIZE;
    }

    // FIXME: Major problem: we need to check for overlapping regions
    // here. If the user asks for the same memory twice. If the user
    // asks for an address out of the range of addresses allowed in
    // userspace, etc.

    // Add memory region to current process
    // TODO: Convert given flags to Memory::PageTableFlag
    // TODO: Figure out what flags we are given (libc, ig).
    process->add_memory_region(address, paddr, size, flags);

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
    DBGMSG("  address: {}\n"
           "\n"
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
    DBGMSG("  tm: {}\n"
           "\n"
           , (void*) time
           );
    if (!time) { return; }
    // FIXME: Validate time pointer
    Time::fill_tm(time);
}


/// Wait for process with PID to terminate. If process with PID is
/// invalid, return immediately.
int sys$9_waitpid(pid_t pid) {
    CPUState* cpu = nullptr;
    asm volatile ("mov %%r11, %0\n"
                  : "=r"(cpu)
                  );
    DBGMSG(sys$_dbgfmt, 9, "waitpid");

    auto* thisProcess = Scheduler::CurrentProcess->value();
    pid_t thisPID = thisProcess->ProcessID;

    // Reap zombie.
    auto zombie = std::find_if(thisProcess->Zombies.begin(), thisProcess->Zombies.begin(), [&pid](const auto& zombie) {
        return zombie.PID == pid;
    });
    if (zombie != thisProcess->Zombies.end()) {
        DBGMSG("[SYS$]:waitpid: Reaping zombie ({}, {}) from process {}\n", zombie->PID, zombie->ReturnStatus, thisPID);
        int returnStatus = zombie->ReturnStatus;
        thisProcess->Zombies.erase(zombie);
        return returnStatus;
    }

    Process *process = Scheduler::process(pid);
    // Return immediately if PID isn't valid.
    // FIXME: Return meaningful value here, or something.
    if (!process) {
        std::print("[SYS$]:ERROR:waitpid: Could not find process at PID {}\n", pid);
        return -1;
    }

    DBGMSG("  pid {} waiting on {}\n\n", thisPID, pid);
    // Add to WAITING list of process that we are waiting for.
    process->WaitingList.push_back(thisPID);

    // Save cpu state into process cache so that we return to the
    // proper place when set off running again.
    memcpy(&Scheduler::CurrentProcess->value()->CPU, cpu, sizeof(CPUState));
    Scheduler::CurrentProcess->value()->State = Process::ProcessState::SLEEPING;
    Scheduler::yield();
}

/// Copy the current process, resuming execution in both just after the
/// fork call, but with the return value to each different (child gets
/// zero, parent gets child's PID).
pid_t sys$10_fork() {
    CPUState* cpu = nullptr;
    asm volatile ("mov %%r11, %0\n"
                  : "=r"(cpu)
                  );
    DBGMSG(sys$_dbgfmt, 10, "fork");
    Process *process = Scheduler::CurrentProcess->value();
    // Use userspace stack pointer instead of kernel stack pointer
    cpu->RSP = cpu->Frame.sp;
    // Save cpu state into process cache so that it will be set
    // properly for the forked process.
    memcpy(&process->CPU, cpu, sizeof(CPUState));
    // Copy current process.
    pid_t cpid = CopyUserspaceProcess(process);
    DBGMSG("  CPID: {}\n", cpid);
    return cpid;
}

/// Replace the current process with a new process, specified by an
/// executable found at PATH.
/// @param path
///   The filepath to the executable that will replace the current
///   process.
/// @param args
///   NULL-terminated array of pointers to NULL-terminated string
///   arguments.
// FIXME: This really needs to be updated to a type-safe API. This stinks like scanf().
void sys$11_exec(const char *path, const char **args) {
    CPUState* cpu = nullptr;
    asm volatile ("mov %%r11, %0\n"
                  : "=r"(cpu)
                  );
    DBGMSG(sys$_dbgfmt, 11, "exec");
    if (!path) {
        std::print("[EXEC]: Can not execute NULL path\n");
        return;
    }
    Process* process = Scheduler::CurrentProcess->value();

    { // Nested scope so that dtors get called before yield
#if defined(DEBUG_SYSCALLS)
    std::print("  path: {}\n"
               "  args:\n"
               , path
               );
    usz i = 0;
    for (const char **args_it = args; args_it && *args_it; ++args_it)
        std::print("    {}: \"{}\"\n", i++, *args_it);
    std::print("  endargs\n");
#endif
        // Load executable at path with virtual filesystem.
        FileDescriptors fds = SYSTEM->virtual_filesystem().open(path);
        if (fds.invalid()) {
            std::print("[EXEC]: Could not load file when path == {}\n", path);
            return;
        }

        process->ExecutablePath = path;

        std::vector<std::string> args_vector_impl;
        std::vector<std::string_view> args_vector;
        args_vector.push_back(process->ExecutablePath.data());
        {   // We create copies of the userspace buffer(s), because during
            // replacing the userspace process, any data within it is
            // invalidated.
            for (const char **args_it = args; args_it && *args_it; ++args_it)
                args_vector_impl.push_back(*args_it);

            for (const auto& s : args_vector_impl)
            args_vector.push_back(s);
        }

        // Replace current process with new process.
        bool success = ELF::ReplaceUserspaceElf64Process(process, fds.Process, args_vector);
        if (!success) {
            // TODO: ... Unrecoverable, terminate the program, somehow.
            std::print("[EXEC]: Failed to replace process and parent is now unrecoverable, terminating.\n");
            // TODO: Mark for destruction (halt and catch fire).
            // FIXME: We should figure out how to exit the scope, so that everything is freed properly.
            process->State = Process::ProcessState::SLEEPING;
            Scheduler::yield();
        }

        //Scheduler::print_debug();
        SYSTEM->virtual_filesystem().close(fds.Process);
    }

    *cpu = process->CPU;
    Scheduler::yield();
}

/// The second file descriptor given will be associated with the file
/// description of the first.
void sys$12_repfd(ProcessFileDescriptor fd, ProcessFileDescriptor replaced) {
    DBGMSG(sys$_dbgfmt, 12, "repfd");
    DBGMSG("  fd: {}, replaced: {}\n\n", fd, replaced);
    Process* process = Scheduler::CurrentProcess->value();
    bool result = SYSTEM->virtual_filesystem().dup2(process, fd, replaced);
    if (!result) {
        std::print("  ERROR OCCURED: repfd failed (pid={}  fd={}  replaced={})\n", process->ProcessID, fd, replaced);
        for (const auto& [procfd, sysfd] : process->FileDescriptors.pairs())
            std::print("  {}: {}\n", procfd, sysfd);
    }
    // TODO: Use result/handle error in some way.
    (void)result;
}

/// Create two file descriptors. One of which can be read from, and the
/// other which can be written to.
void sys$13_pipe(ProcessFileDescriptor *fds) {
    DBGMSG(sys$_dbgfmt, 13, "pipe");
    // TODO: Validate pointer.
    Process* process = Scheduler::CurrentProcess->value();
    VFS& vfs = SYSTEM->virtual_filesystem();
    // Lay down a new pipe.
    auto pipeEnds = vfs.PipesDriver->lay_pipe();
    FileDescriptors readFDs = vfs.add_file(std::move(pipeEnds.Read), process);
    FileDescriptors writeFDs = vfs.add_file(std::move(pipeEnds.Write), process);
    // Write fds.
    fds[0] = readFDs.Process;
    fds[1] = writeFDs.Process;
}


/// OFFSET is based off of current offset (relative).
#define SEEK_CUR 0
/// OFFSET is from end of file (must be negative).
#define SEEK_END 1
/// OFFSET is from beginning of file (must be positive).
#define SEEK_SET 2
static const char * seek_strings[] = {
    [SEEK_CUR] = "CURRENT",
    [SEEK_END] = "END",
    [SEEK_SET] = "BEGINNING",
};
const char *get_seek_string(int whence) {
    if (whence < 0 || whence > SEEK_SET) return "INVALID";
    return seek_strings[whence];
}

// TODO: Thread safety is out of the window.
// TODO: Should probably allow for setting offset past file size
// (requires implementing holes, I think).
/// @param offset    Byte offset to set within file.
/// @param whence    One of SEEK_* directives
/// @return 1 if offset is outside of file bounds, whence is invalid,
///         or fd is invalid. Otherwise, return zero.
int sys$14_seek(ProcessFileDescriptor fd, ssz offset, int whence) {
    DBGMSG(sys$_dbgfmt, 14, "seek");
    DBGMSG("[SYS$]:seek(): {}, offset={}, whence={}\n", fd, offset, get_seek_string(whence));

    VFS& vfs = SYSTEM->virtual_filesystem();
    std::shared_ptr<FileMetadata> file = vfs.file(fd);
    if (!file) return 1;

    switch (whence) {
    case SEEK_CUR: {
        if (offset == 0) return 0;
        usz current_offset = file.get()->offset;
        // Cannot seek behind beginning of file...
        if (offset < 0 && ((usz)(-offset) > current_offset)) return 1;
        // FIXME: Cannot seek past end of file...
        else if (current_offset + offset > file.get()->file_size()) return 1;
        file.get()->offset += offset;
    } return 0;

    case SEEK_END: {
        // FIXME: Cannot seek past end of file...
        if (offset > 0) return 1;
        file.get()->offset = file.get()->file_size() + offset;
    } return 0;

    case SEEK_SET: {
        if (offset < 0) return 1;
        // FIXME: Cannot seek past end of file...
        if ((usz)offset >= file.get()->file_size()) return 1;
        file.get()->offset = offset;
    } return 0;

    default: break;

    }

    return 1;
}

/// Write as much of the current working directory to the given buffer
/// while not exceeding numBytes (including null terminator)
/// Return true iff entire PWD was written.
bool sys$15_pwd(char *buffer, usz numBytes) {
    DBGMSG(sys$_dbgfmt, 15, "pwd");
    DBGMSG("[SYS$]:pwd(): buffer={}, numBytes={}\n", (void*)buffer, numBytes);

    if (!buffer || !numBytes)
        return false;

    Process *process = Scheduler::CurrentProcess->value();
    bool entire = numBytes > process->WorkingDirectory.size();

    DBGMSG("  PID:{} pwd: \"{}\"\n", process->ProcessID, process->WorkingDirectory);

    usz byteCount = entire ? process->WorkingDirectory.size() + 1 : numBytes;
    memcpy(buffer, process->WorkingDirectory.data(), byteCount);
    buffer[byteCount - 1] = '\0';

    return entire;
}

/// The returned file descriptor will be associated with the file
/// description of the given file descriptor.
ProcFD sys$16_dup(ProcessFileDescriptor fd) {
    DBGMSG(sys$_dbgfmt, 16, "dup");
    DBGMSG("  fd: {}\n", fd);
    auto* process = Scheduler::CurrentProcess->value();
    auto fds = SYSTEM->virtual_filesystem().dup(process, fd);
    if (fds.invalid())
        std::print("  ERROR OCCURED: dup failed\n");
    return fds.Process;
}


// TODO: Reorder this
// FIXME: Make it easier to reorder this (maybe separate the number
// from the name? I don't know, something to make this easier...)
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

    // MORE FILE STUFFS
    (void*)sys$14_seek,

    (void*)sys$15_pwd,

    (void*)sys$16_dup,
};
