#include "file.h"
#include <interrupts/syscalls.h>

#include <debug.h>
#include <system.h>
#include <virtual_filesystem.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_SYSCALLS

/// SYSCALL NAMING SCHEME:
/// "sys$" + number + "_" + descriptive name

constexpr const char* sys$_dbgfmt = "[SYS$]: %d -- %s\r\n";

int sys$0_open(const char* path) {
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

void sys$2_read() {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 2, "read");
#endif /* #ifdef DEBUG_SYSCALLS */
}

int sys$3_write(FileDescriptor fd, u8* buffer, u64 byteCount) {
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 3, "write");
#endif /* #ifdef DEBUG_SYSCALLS */
    // TODO: Implement seek behaviour.
    dbgmsg("  file descriptor: %d\r\n"
           "  buffer address:  %x\r\n"
           "  byte count:      %ull\r\n"
           "\r\n"
           , fd
           , buffer
           , byteCount
           );
    return SYSTEM->virtual_filesystem().write(fd, buffer, byteCount, 0);
}

void sys$4_poke() {
    // Prevent unused warning
    (void)sys$_dbgfmt;
#ifdef DEBUG_SYSCALLS
    dbgmsg(sys$_dbgfmt, 4, "poke");
#endif /* #ifdef DEBUG_SYSCALLS */
    dbgmsg_s("Poke from userland!\r\n");
}

u64 num_syscalls = LENSOR_OS_NUM_SYSCALLS;
void* syscalls[LENSOR_OS_NUM_SYSCALLS] = {
    (void*)sys$0_open,
    (void*)sys$1_close,
    (void*)sys$2_read,
    (void*)sys$3_write,
    (void*)sys$4_poke,
};
