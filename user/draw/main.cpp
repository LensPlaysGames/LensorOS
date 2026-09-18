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
                } break;

                default:
                    break;
            }
        }

        if (left_click_pressed)
            draw_pixel(*framebuffer, cursor_x, cursor_y, color);
        else if (right_click_pressed)
            draw_pixel(*framebuffer, cursor_x, cursor_y, BLACK);

        std::sys_cooperative_yield();
    }

    std::print("[DRAW]: shutting down...\n");

    // Get rid of GUI window stuffs
    gui_teardown(gui);

    std::print("[DRAW]: bye bye!\n");
}
