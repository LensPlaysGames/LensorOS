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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#include <basic_renderer.h>
#include <integers.h>
#include <io.h>
#include <math.h>
#include <mouse.h>
#include <random_lfsr.h>

#include <format>

u8 gMouseID;

void mouse_wait() {
    u64 timeout = 100000;
    while (timeout--) {
        if ((in8(0x64) & 0b10) == 0)
            return;
    }
}

void mouse_wait_input() {
    u64 timeout = 100000;
    while (timeout--) {
        if (in8(0x64) & 0b1)
            return;
    }
}

void mouse_write(u8 value) {
    mouse_wait_input();
    out8(0x64, 0xd4);
    mouse_wait();
    out8(0x60, value);
}

u8 mouse_read() {
    mouse_wait_input();
    return in8(0x60);
}

void init_ps2_mouse() {
    // Enable mouse.
    out8(0x64, 0xA8);
    mouse_wait();
    // Tell keyboard controller a mouse message is incoming.
    out8(0x64, 0x20);

    mouse_wait_input();
    u8 status = in8(0x60);
    status |= 0b10;
    mouse_wait();
    out8(0x64, 0x60);
    mouse_wait();
    out8(0x60, status);

    mouse_write(0xf6);
    mouse_read();  // ACKNOWLEDGE

    mouse_write(0xf4);
    mouse_read();  // ACK

    mouse_write(0xf2);
    mouse_read();  // ACK
    gMouseID = mouse_read();

    std::print("[Mouse]: Successfully initialized PS2 mouse using serial port (ID: {})\n", gMouseID);
}
