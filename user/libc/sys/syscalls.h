#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include <stdint.h>

#define SYS_open  0
#define SYS_close 1
#define SYS_read  2
#define SYS_write 3
#define SYS_poke  4
#define SYS_exit  5

#define SYS_MAXSYSCALL 6

#if defined (__cplusplus)
extern "C" {
#endif
    // TODO: Preprocessor macro trickery.
    uintptr_t syscall(uintptr_t systemCall);
#if defined (__cplusplus)
} /* extern "C" */
#endif

#if defined (__cplusplus)
template<typename T0>
inline uintptr_t syscall(uintptr_t systemCall, T0 arg0) {
    uintptr_t result;
    asm volatile("int $0x80"
                 : "=a"(result)
                 : "a"(systemCall)
                   , "D"(arg0)
                 : "memory"
                 );
    return result;
}

template<typename T0, typename T1>
inline uintptr_t syscall(uintptr_t systemCall, T0 arg0, T1 arg1) {
    uintptr_t result;
    asm volatile("int $0x80"
                 : "=a"(result)
                 : "a"(systemCall)
                   , "D"(arg0)
                   , "S"(arg1)
                 : "memory"
                 );
    return result;
}

template<typename T0, typename T1, typename T2>
inline uintptr_t syscall(uintptr_t systemCall, T0 arg0, T1 arg1, T2 arg2) {
    uintptr_t result;
    asm volatile("int $0x80"
                 : "=a"(result)
                 : "a"(systemCall)
                   , "D"(arg0)
                   , "S"(arg1)
                   , "d"(arg2)
                 : "memory"
                 );
    return result;
}

template<typename T0, typename T1, typename T2, typename T3>
inline uintptr_t syscall(uintptr_t systemCall, T0 arg0, T1 arg1, T2 arg2, T3 arg3) {
    uintptr_t result;
    // Arguments passed in RDI, RSI, RDX, RCX
    asm volatile("int $0x80"
                 : "=a"(result)
                 : "a"(systemCall)
                   , "D"(arg0)
                   , "S"(arg1)
                   , "d"(arg2)
                   , "c"(arg3)
                 : "memory"
                 );
    return result;
}
#endif /* #if defined (__cplusplus) */

#endif /* #ifndef _SYSCALLS_H */
