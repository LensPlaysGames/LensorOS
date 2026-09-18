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

#ifndef LENSOROS_DEFINES_IPC_H
#define LENSOROS_DEFINES_IPC_H

#include <stdint.h>

#define IPC_MAX_SIZE 32

#define IPC_KEYBOARD_MAGIC 0xf8
typedef struct ipc_keyboard_t {
    uint8_t magic;
    uint8_t is_pressed;
    uint16_t value;
} ipc_keyboard_t;
static_assert(sizeof(ipc_keyboard_t) <= IPC_MAX_SIZE);

#define IPC_MOUSE_POSITION_MAGIC 0xf9
typedef struct ipc_mouse_position_t {
    uint8_t magic;
    int32_t local_x;
    int32_t local_y;
} ipc_mouse_position_t;
static_assert(sizeof(ipc_mouse_position_t) <= IPC_MAX_SIZE);

#define IPC_MOUSE_DELTA_MAGIC 0xfa
typedef struct ipc_mouse_delta_t {
    uint8_t magic;
    int32_t delta_x;
    int32_t delta_y;
} ipc_mouse_delta_t;
static_assert(sizeof(ipc_mouse_delta_t) <= IPC_MAX_SIZE);

#endif /* LENSOROS_DEFINES_IPC_H */
