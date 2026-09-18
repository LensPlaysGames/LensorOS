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

#ifndef LENSOROS_DEFINES_SYSCALLS_H
#define LENSOROS_DEFINES_SYSCALLS_H

typedef enum LensorOS_SyscallRead_Flags {
    LENSOROS_SYSCALL_READ_FLAG_NONE = 0,
    // If this bit is set, do *not* block to wait for data.
    LENSOROS_SYSCALL_READ_FLAG_NOBLOCK = 1 << 0,
} LensorOS_SyscallRead_Flags;

typedef enum LensorOS_SyscallWrite_Flags {
    LENSOROS_SYSCALL_WRITE_FLAG_NONE = 0,
    // If this bit is set, do *not* block to wait for room to write data.
    LENSOROS_SYSCALL_WRITE_FLAG_NOBLOCK = 1 << 0,
} LensorOS_SyscallWrite_Flags;

#endif /* LENSOROS_DEFINES_SYSCALLS_H */
