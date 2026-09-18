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

#ifndef LENSOROS_LIBGUICLIENT_GUI_H
#define LENSOROS_LIBGUICLIENT_GUI_H

#include <bits/decls.h>
#include <lensor/ipc.h>
#include <lensor/keys.h>
#include <stddef.h>
#include <stdint.h>

__BEGIN_DECLS__

typedef struct gui_framebuffer_t {
    // linear framebuffer's base address
    uintptr_t base_address;
    // first invalid index to access offset of the linear framebuffer's base
    // address
    uintptr_t buffer_size;
    // size of x-axis of visible window
    uintptr_t pixel_width;
    // size of y-axis of visible window
    uintptr_t pixel_height;
    // pixel width is visible, but this is the actual memory layout
    uintptr_t pixels_per_line;
    // byte count that a single pixel takes up in the linear framebuffer
    uintptr_t pixel_byte_width;
} gui_framebuffer_t;

typedef struct gui_info_t {
    gui_framebuffer_t framebuffer;
    // socket file descriptor that we talk to server with
    uintptr_t client_file_descriptor;
} gui_info_t;

// @return handle
uintptr_t gui_startup();
gui_framebuffer_t* gui_get_framebuffer(uintptr_t handle);
// non-blocking. if event is ready, fill event parameter with event.
// @return true iff event has valid data.
bool gui_get_event(uintptr_t handle, uint8_t event[IPC_MAX_SIZE]);
void gui_teardown(uintptr_t handle);

__END_DECLS__

#endif /* LENSOROS_LIBGUICLIENT_GUI_H */
