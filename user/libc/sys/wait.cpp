/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

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
