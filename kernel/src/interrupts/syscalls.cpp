#include "syscalls.h"

#include "../basic_renderer.h"
#include "../cstr.h"
#include "../file.h"
#include "../uart.h"

/// Get a FileDescriptor that may be used in subsequent
/// syscalls (`read`, `write`, etc) from a file path.
void sys$open(const char* path, int flags) {
    /* TODO:
     * |-- Resolve file from path using VFS.
     * `-- Create open file description in system-wide table.
     */
    (void)path;
    (void)flags;

    UART::out("[SYS$]: `open`  path=");
    UART::out(path);
    UART::out(", flags=");
    UART::out(to_string(flags));
    UART::out("\r\n");

    // TODO: Figure out how to return FileDescriptor!
}

/// Close the OpenFileDescription pointed to by `fd`.
void sys$close(FileDescriptor fd) {
    /* TODO:
     * |-- Find open file description entry in system-wide table.
     * `-- Either remove the entry, or mark it for removal.
     */
    (void)fd;

    UART::out("[SYS$]: `close`  fd=");
    UART::out((u64)fd);
    UART::out("\r\n");
}

/// Read `numBytes` bytes of the contents of `buffer`
/// to the OpenFileDescription pointed to by `fd`.
void sys$read(FileDescriptor fd, void* buffer, u64 numBytes) {
    /* TODO:
     * |-- Resolve open file desc. using `fd` as index into system-wide table.
     * `-- Call read on the open file description.
     */
    (void)fd;
    (void)buffer;
    (void)numBytes;

    UART::out("[SYS$]: `read`  fd=");
    UART::out((u64)fd);
    UART::out(", buffer=0x");
    UART::out(to_hexstring(buffer));
    UART::out(", numBytes=");
    UART::out(numBytes);
    UART::out("\r\n");
}

/// Write `numBytes` bytes of the contents of `buffer`
/// to the OpenFileDescription pointed to by `fd`.
void sys$write(FileDescriptor fd, void* buffer, u64 numBytes) {
    /* TODO:
     * |-- Resolve open file desc. using `fd` as index into system-wide table.
     * `-- Call write on the open file description.
     */
    (void)fd;
    (void)buffer;
    (void)numBytes;

    UART::out("[SYS$]: `write`  fd=");
    UART::out((u64)fd);
    UART::out(", buffer=0x");
    UART::out(to_hexstring(buffer));
    UART::out(", numBytes=");
    UART::out(numBytes);
    UART::out("\r\n");
}

void sys$0_test0() {
    UART::out("[SYS$]: System call 'test0'\r\n");
}

void sys$1_test1() {
    UART::out("[SYS$]: System call 'test1'\r\n");
}

u64 num_syscalls = LENSOR_OS_NUM_SYSCALLS;
void* syscalls[LENSOR_OS_NUM_SYSCALLS] = {
    (void*)sys$0_test0,
    (void*)sys$1_test1
};
