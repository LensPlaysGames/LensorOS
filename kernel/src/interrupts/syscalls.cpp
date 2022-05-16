#include <interrupts/syscalls.h>

#include <debug.h>

/// SYSCALL NAMING SCHEME:
/// "sys$" + number + "_" + descriptive name

constexpr const char* sys$_dbgfmt = "[SYS$]: %d -- ";

void sys$0_open() {
    dbgmsg(sys$_dbgfmt, 0);
}

void sys$1_close() {
    dbgmsg(sys$_dbgfmt, 1);
}

u64 num_syscalls = LENSOR_OS_NUM_SYSCALLS;
void* syscalls[LENSOR_OS_NUM_SYSCALLS] = {
    (void*)sys$0_open,
    (void*)sys$1_close,
};
