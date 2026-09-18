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

#include <guiclient/gui.h>
#include <lensor/files.h>
#include <lensor/ipc.h>
#include <lensor/syscalls.h>
#include <sys/syscalls.h>

bool gui_get_event(uintptr_t handle, uint8_t event[IPC_MAX_SIZE]) {
    if (not handle) return false;
    gui_info_t* info = (gui_info_t*)handle;
    if (info->client_file_descriptor == (uintptr_t)ProcFD::Invalid)
        return false;

    // non-blocking read
    ssize_t ipc_bytes_read = std::sys_read(
        info->client_file_descriptor,
        &event[0],
        1,
        LENSOROS_SYSCALL_READ_FLAG_NOBLOCK);

    if (ipc_bytes_read <= 0)
        return false;

    uint8_t magic = event[0];
    ssize_t read_size = sizeof(magic);
    switch (magic) {
        case IPC_KEYBOARD_MAGIC:
            read_size = sizeof(ipc_keyboard_t);
            break;

        case IPC_MOUSE_DELTA_MAGIC:
            read_size = sizeof(ipc_mouse_delta_t);
            break;

        case IPC_MOUSE_POSITION_MAGIC:
            read_size = sizeof(ipc_mouse_position_t);
            break;

        default:
            return false;
    }
    read_size -= sizeof(magic);

    // event with no data, just tag.
    if (not read_size) return true;

    ssize_t ipc_data_bytes_read = std::sys_read(
        info->client_file_descriptor,
        &event[1],
        read_size,
        LENSOROS_SYSCALL_READ_FLAG_NOBLOCK);

    if (ipc_data_bytes_read != read_size)
        return false;

    return true;
}
