#include <errno.h>
#include <stdio.h>

#include "sys/wait.h"

#if defined(__lensor__)
#include "sys/syscalls.h"
#elif defined(__unix__)
#include "sys/syscall.h"
#endif

pid_t waitpid(pid_t pid, int *wstatus, int options) {
    int command_status = syscall<int>(SYS_waitpid, pid);
    if (command_status == -1) {
        errno = EFAULT;
        return -1;
    }
    if (wstatus)
        *wstatus = command_status;
    return 0;
}
