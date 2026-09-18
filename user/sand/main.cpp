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

constexpr uint32_t WIDTH = 640;
constexpr uint32_t HEIGHT = 480;
constexpr uint32_t COLOR(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff) {
    return ((uint32_t)a << 24)
           | (((uint32_t)r) << 16)
           | (((uint32_t)g) << 8)
           | ((uint32_t)b);
}
// ARGB
constexpr uint32_t SAND_COLOR = COLOR(0xff, 0xff, 0xff);
// ARGB
constexpr uint32_t EMPTY_COLOR = COLOR(0x00, 0x00, 0x00);

enum struct Kind : uint8_t {
    Empty,
    Sand
};

uint8_t grid[WIDTH][HEIGHT]{};

// Update the sand physics
void update_sand_simulation() {
    // Loop backwards over a window of two rows, bottom-to--top so falling
    // sand doesn't accelerate instantly in one frame.
    for (int y = HEIGHT - 2; y >= 0; --y) {
        for (int x = 0; x < WIDTH; ++x) {
            // If it's a sand particle
            if (grid[x][y] == (uint8_t)Kind::Sand) {
                // Can we move straight down?
                if (grid[x][y + 1] == 0) {
                    grid[x][y] = 0;
                    grid[x][y + 1] = 1;
                }
                // Can we fall diagonally left?
                else if (x > 0 && grid[x - 1][y + 1] == 0) {
                    grid[x][y] = 0;
                    grid[x - 1][y + 1] = 1;
                }
                // Can we fall diagonally right?
                else if (x < WIDTH - 1 && grid[x + 1][y + 1] == 0) {
                    grid[x][y] = 0;
                    grid[x + 1][y + 1] = 1;
                }
            }
        }
    }
}

void draw_grid(const gui_framebuffer_t& framebuffer) {
    auto* const base = (uint8_t*)framebuffer.base_address;
    const uintptr_t line_pitch = framebuffer.pixels_per_line * framebuffer.pixel_byte_width;
    const int height = std::min((uint32_t)framebuffer.pixel_height, HEIGHT);
    const int width = std::min((uint32_t)framebuffer.pixel_width, WIDTH);
    for (int y = 0; y < height; ++y) {
        uint8_t* const row = (uint8_t*)(base + (y * line_pitch));
        for (int x = 0; x < width; ++x) {
            uint32_t* const pixel = (uint32_t*)(row + (x * framebuffer.pixel_byte_width));
            if (grid[x][y] == (uint8_t)Kind::Sand) {
                *pixel = SAND_COLOR;
            }
            else if (grid[x][y] == (uint8_t)Kind::Empty) {
                *pixel = EMPTY_COLOR;
            }
        }
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
    draw_grid(*framebuffer);

    bool running{true};
    bool click_pressed{};
    int32_t cursor_x{};
    int32_t cursor_y{};
    while (running) {
        uint8_t event[IPC_MAX_SIZE];
        if (gui_get_event(gui, event)) {
            switch (event[0]) {
                case IPC_KEYBOARD_MAGIC: {
                    auto* keyboard_event = (ipc_keyboard_t*)&event[0];
                    // make point under cursor sand on left click
                    if (keyboard_event->value == LENSOR_KEY_MOUSE_LEFT)
                        click_pressed = keyboard_event->is_pressed;

                    // ignore releases
                    if (not keyboard_event->is_pressed) break;

                    if (keyboard_event->value == LENSOR_KEY_ESC) {
                        std::print("[SAND]: preparing for shutdown...\n");
                        running = false;
                    }
                } break;

                case IPC_MOUSE_POSITION_MAGIC: {
                    auto* mouse_event = (ipc_mouse_position_t*)&event[0];
                    cursor_x = mouse_event->local_x;
                    cursor_y = mouse_event->local_y;
                } break;

                default:
                    break;
            }
        }

        if (click_pressed
            and cursor_x < WIDTH
            and cursor_y < HEIGHT) {
            grid[cursor_x][cursor_y] = (uint8_t)Kind::Sand;
        }

        update_sand_simulation();

        draw_grid(*framebuffer);

        std::sys_cooperative_yield();
    }

    std::print("[SAND]: shutting down...\n");

    // Get rid of GUI window stuffs
    gui_teardown(gui);

    std::print("[SAND]: bye bye!\n");
}
