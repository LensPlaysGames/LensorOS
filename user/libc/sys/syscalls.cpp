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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>.
 */


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