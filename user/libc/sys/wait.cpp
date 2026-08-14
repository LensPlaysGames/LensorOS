#include "sys/wait.h"

#include <errno.h>
#include <stdio.h>

#if defined(__lensor__)
#include "sys/syscalls.h"
#elif defined(__unix__)
#include "sys/syscall.h"
#endif

extern "C" pid_t waitpid(pid_t pid, int* wstatus, int options) {
    int command_status = syscall<int>(SYS_waitpid, pid, wstatus);
    if (command_status == -1) {
        errno = EFAULT;
        return -1;
    }
    if (wstatus)
        *wstatus = WEXITSTATUS(*wstatus);
    return 0;
}
