#include "syscalls.h"

#include "../basic_renderer.h"
#include "../uart.h"

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
