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

#include <stdint.h>

typedef enum hardware_query_type_t {
    LENSOR_HARDWARE_GRAPHIC,
    LENSOR_HARDWARE_AUDIO,

    LENSOR_HARDWARE_COUNT
} hardware_query_type_t;
typedef struct hardware_graphic_t {
    uintptr_t fb_width;
    uintptr_t fb_height;
    uintptr_t fb_bytes_per_line;
    uintptr_t fb_size;
    uint8_t fb_format;
    void* fb_base; /**< linear framebuffer mapped into process' memory region */
} hardware_graphic_t;
typedef struct hardware_audio_t {
    void* sample_buffers[8];
    uint8_t sample_buffer_count;
    uintptr_t sample_buffer_byte_size;
} hardware_audio_t;

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
