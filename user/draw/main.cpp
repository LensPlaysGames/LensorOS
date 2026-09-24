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
#include <stdint.h>
#include <sys/syscalls.h>

#include <algorithm>
#include <print>

// ARGB
constexpr uint32_t COLOR(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff) {
    return ((uint32_t)a << 24)
           | (((uint32_t)r) << 16)
           | (((uint32_t)g) << 8)
           | ((uint32_t)b);
}
constexpr uint32_t WHITE = COLOR(0xff, 0xff, 0xff);
constexpr uint32_t BLACK = COLOR(0x00, 0x00, 0x00);

enum struct Kind : uint8_t {
    Empty,
    Sand
};

void draw_pixel(
    const gui_framebuffer_t& framebuffer,
    size_t x,
    size_t y,
    const uint32_t color) {
    auto* const base = (uint8_t*)framebuffer.base_address;
    const uintptr_t line_pitch = framebuffer.pixels_per_line * framebuffer.pixel_byte_width;
    y = std::min(framebuffer.pixel_height - 1, y);
    x = std::min(framebuffer.pixel_width - 1, x);
    uint8_t* const row = (uint8_t*)(base + (y * line_pitch));
    uint32_t* const pixel = (uint32_t*)(row + (x * framebuffer.pixel_byte_width));
    *pixel = color;
}

void draw_rectangle(
    const gui_framebuffer_t& framebuffer,
    size_t x,
    size_t y,
    size_t w,
    size_t h,
    const uint32_t color) {
    // Exit if the rectangle is entirely off-screen or has zero area
    if (x >= framebuffer.pixel_width - 1
        or y >= framebuffer.pixel_height - 1
        or w == 0 or h == 0) {
        return;
    }

    // Calculate boundaries, clipping against screen edges
    const size_t x_end = std::min(framebuffer.pixel_width, x + w);
    const size_t y_end = std::min(framebuffer.pixel_height, y + h);

    auto* const base = (uint8_t*)framebuffer.base_address;
    const uintptr_t bytes_per_line = framebuffer.pixels_per_line * framebuffer.pixel_byte_width;

    // Loop through each row
    for (size_t current_y = y; current_y < y_end; ++current_y) {
        // Find the start of the current row
        auto* const row = (uint8_t*)(base + (current_y * bytes_per_line));

        // Find the start pixel in this row
        auto* const row_start_pixel = (uint32_t*)(row + (x * framebuffer.pixel_byte_width));

        // Loop through each pixel in the row
        const size_t row_width = x_end - x;
        for (size_t current_x = 0; current_x < row_width; ++current_x)
            row_start_pixel[current_x] = color;
    }
}

int main(int argc, const char** argv) {
    // Get GUI Window Stuffs
    auto gui = gui_startup();
    if (not gui) {
        std::print("[SAND]: could not initialize GUI\n");
        return 1;
    }

    auto* framebuffer = gui_get_framebuffer(gui);

    bool running{true};
    bool left_click_pressed{};
    bool right_click_pressed{};
    int32_t cursor_x{};
    int32_t cursor_y{};
    uint32_t color = WHITE;
    while (running) {
        uint8_t event[IPC_MAX_SIZE];
        if (gui_get_event(gui, event)) {
            switch (event[0]) {
                case IPC_KEYBOARD_MAGIC: {
                    auto* keyboard_event = (ipc_keyboard_t*)&event[0];

                    if (keyboard_event->value == LENSOR_KEY_MOUSE_LEFT)
                        left_click_pressed = keyboard_event->is_pressed;
                    if (keyboard_event->value == LENSOR_KEY_MOUSE_RIGHT)
                        right_click_pressed = keyboard_event->is_pressed;

                    if (not keyboard_event->is_pressed) break;

                    if (keyboard_event->value == LENSOR_KEY_ESC) {
                        std::print("[DRAW]: preparing for shutdown...\n");
                        running = false;
                    }

                    else if (keyboard_event->value == LENSOR_KEY_SPACE)
                        color *= 0xf00fdead;
                } break;

                case IPC_MOUSE_POSITION_MAGIC: {
                    auto* mouse_event = (ipc_mouse_position_t*)&event[0];
                    cursor_x = mouse_event->local_x;
                    cursor_y = mouse_event->local_y;
                    if (left_click_pressed)
                        draw_pixel(*framebuffer, cursor_x, cursor_y, color);
                    else if (right_click_pressed)
                        draw_rectangle(*framebuffer, cursor_x, cursor_y, 12, 12, BLACK);
                } break;

                default:
                    break;
            }
        }

        draw_rectangle(*framebuffer, 0, 0, 12, 12, color);

        std::sys_cooperative_yield();
    }

    std::print("[DRAW]: shutting down...\n");

    // Get rid of GUI window stuffs
    gui_teardown(gui);

    std::print("[DRAW]: bye bye!\n");
}
