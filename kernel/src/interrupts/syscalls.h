#ifndef LENSOR_OS_SYSCALLS_H
#define LENSOR_OS_SYSCALLS_H

#include <integers.h>

constexpr u64 LENSOR_OS_NUM_SYSCALLS = 2;
extern void* syscalls[LENSOR_OS_NUM_SYSCALLS];

// Defined in `syscalls.cpp`
// Used by `syscalls.asm`
extern u64 num_syscalls;

// Defined in `syscalls.asm`
extern "C" void system_call_handler_asm();

#endif
