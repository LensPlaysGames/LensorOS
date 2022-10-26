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
#include <scheduler.h>
#include <system.h>
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
    return SYSTEM->virtual_filesystem().open(path);
}

void sys$1_close(FileDescriptor fd) {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 1, "close");
#endif /* #ifdef DEBUG_SYSCALLS */
    SYSTEM->virtual_filesystem().close(fd);
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
    return SYSTEM->virtual_filesystem().read(fd, buffer, byteCount, 0);
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
    return SYSTEM->virtual_filesystem().write(fd, buffer, byteCount, 0);
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
    dbgmsg("[SYS$]: exit() -- Removed process %ull\r\n", pid);
    (void)status;
}

u64 num_syscalls = LENSOR_OS_NUM_SYSCALLS;
void* syscalls[LENSOR_OS_NUM_SYSCALLS] = {
    (void*)sys$0_open,
    (void*)sys$1_close,
    (void*)sys$2_read,
    (void*)sys$3_write,
    (void*)sys$4_poke,
    (void*)sys$5_exit,
};
