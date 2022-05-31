#include <interrupts/syscalls.h>

#include <debug.h>
#include <file.h>
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

u64 num_syscalls = LENSOR_OS_NUM_SYSCALLS;
void* syscalls[LENSOR_OS_NUM_SYSCALLS] = {
    (void*)sys$0_open,
    (void*)sys$1_close,
    (void*)sys$2_read,
    (void*)sys$3_write,
    (void*)sys$4_poke,
};
