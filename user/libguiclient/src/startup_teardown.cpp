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
#include <stdint.h>
#include <sys/syscalls.h>

gui_info_t gGUIINFO{};

gui_framebuffer_t* gui_get_framebuffer(uintptr_t handle) {
    if (not handle) return nullptr;
    auto* info = (gui_info_t*)handle;
    return &info->framebuffer;
}

uintptr_t gui_startup() {
    gui_info_t* info = &gGUIINFO;

    // Get graphical window from opening a connection to the !GUI socket
    info->client_file_descriptor = std::sys_socket(0, 0, 0);
    sockaddr addr{sockaddr::LENSOR16, "!GUI"};
    auto rc = std::sys_connect(
        info->client_file_descriptor,
        &addr,
        sizeof(addr));
    if (rc) {
        std::sys_close(info->client_file_descriptor);
        printf("[TERM]: Couldn't connect to GUI Server (address: %s)\n", addr.data);
        fflush(stdout);
        return 0;
    }
    unsigned char data[32];
    ssize_t bytes_read = std::sys_read(
        info->client_file_descriptor,
        data,
        24,
        0);
    uint64_t* data_it = (uint64_t*)data;
    uint64_t shared_memory_id = data_it[2];
    uintptr_t* shared_data = (uintptr_t*)syscall(
        SYS_shared_memory_acquire,
        shared_memory_id);

    auto& framebuffer = info->framebuffer;
    const auto& initial_info = (const initial_shared_memory_state_t&)*shared_data;
    framebuffer.base_address = (uintptr_t)shared_data;
    framebuffer.buffer_size = initial_info.fb_size;
    framebuffer.pixel_width = initial_info.fb_width;
    framebuffer.pixel_height = initial_info.fb_height;
    // TODO: Use format passed from GUI server
    framebuffer.pixels_per_line = initial_info.fb_bytes_per_line / 4;
    framebuffer.pixel_byte_width = 4;

    // clear visual artifact from passing data in shared memory region used by
    // framebuffer.
    memset(shared_data, 0, (size_t)bytes_read);

    return (uintptr_t)&gGUIINFO;
}

void gui_teardown(uintptr_t handle) {
    if (not handle) return;
    gui_info_t* info = (gui_info_t*)handle;

    if (info->framebuffer.base_address)
        syscall(SYS_shared_memory_release, info->framebuffer.base_address);
    info->framebuffer.base_address = 0;

    if (info->client_file_descriptor != (uintptr_t)ProcFD::Invalid)
        std::sys_close(info->client_file_descriptor);
    info->client_file_descriptor = (uintptr_t)ProcFD::Invalid;
}
