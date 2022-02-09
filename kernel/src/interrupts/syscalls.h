#ifndef LENSOR_OS_SYSCALLS_H
#define LENSOR_OS_SYSCALLS_H

#include "../integers.h"

#define LENSOR_OS_NUM_SYSCALLS 2

/// SYSCALL NAMING SCHEME:
/// "sys$" + number + "_" + descriptive name

void sys$0_test0();
void sys$1_test1();

extern void* syscalls[LENSOR_OS_NUM_SYSCALLS];
extern u64 num_syscalls;

extern "C" void system_call_handler_asm();

#endif
