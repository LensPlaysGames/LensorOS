#include "syscalls.h"

void sys$0_test0() {
    srl->writestr("[SYS$]: System call 'test0'\r\n");
}

void sys$1_test1() {
    srl->writestr("[SYS$]: System call 'test1'\r\n");
}

u64 num_syscalls = LENSOR_OS_NUM_SYSCALLS;
void* syscalls[LENSOR_OS_NUM_SYSCALLS] = {
    (void*)sys$0_test0,
    (void*)sys$1_test1
};
