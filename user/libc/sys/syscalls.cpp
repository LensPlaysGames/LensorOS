#include "syscalls.h"

extern "C" {
    uintptr_t syscall(uintptr_t systemCall) {
        if (systemCall >= SYS_MAXSYSCALL)
            return -1;
        uintptr_t result;
        __asm__ volatile ("int $0x80"
                          : "=a"(result)
                          : "a"(systemCall)
                          : "memory"
                          );
        return result;
    }
}
