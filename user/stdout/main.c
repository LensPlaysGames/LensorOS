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

#include <framebuffer.h>
#include <ints.h>
#include <lensor/keys.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscalls.h>
#include <sys/types.h>
#include <unistd.h>

// The screen itself
static Framebuffer g_framebuffer;
// The canvas that is blitted to the screen
static Framebuffer g_backbuffer;

#define ESCAPE 0x01
#define BACKSPACE 0x0e
#define TAB 0x0f
#define ENTER 0x1c
#define LCONTROL 0x1d
#define LSHIFT 0x2a
#define RSHIFT 0x36
#define LALT 0x38
#define SPACE 0x39
#define CAPSLOCK 0x3a
#define NUMLOCK 0x45
#define SCROLLLOCK 0x46

/// Preceded by 'e0' byte.
#define ARROW_UP 0x48
#define ARROW_DOWN 0x50
#define ARROW_LEFT 0x4b
#define ARROW_RIGHT 0x4d

void fprint_hexnibble(unsigned char byte, FILE* f) {
    if (byte < 10)
        putc(byte + '0', f);
    else if (byte < 16)
        putc(byte - 10 + 'a', f);
    else
        putc('?', f);
}

unsigned int hex_value_digit(unsigned char value) {
    if (value < 10)
        return value + '0';
    else if (value < 16)
        return value - 10 + 'a';
    return -1;
}

void fprint_hexnumber(size_t number, FILE* f) {
    char leading = 1;
    for (size_t i = sizeof(size_t) - 1; i < sizeof(size_t); --i) {
        size_t value = (number >> (i * 8)) & 0xff;
        if (leading && !value)
            continue;
        else
            leading = 0;
        putc(hex_value_digit((number >> (4 + i * 8)) & 0x0f), f);
        putc(hex_value_digit((number >> (i * 8)) & 0x0f), f);
    }
    if (leading) {
        putc('0', f);
    }
}

unsigned int hex_digit_value(const char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
    else if (c >= 'a' && c <= 'f')
        return 10 + c - 'a';
    return -1;
}

size_t hexstring_to_number(const char* str) {
    size_t out = 0;
    // Skip `0x`
    if (*str == '0' && *(str + 1) == 'x') str += 2;

    char c;
    unsigned char val = 0;
    for (size_t i = sizeof(size_t) - 1; i < sizeof(size_t); --i) {
        c = *(str++);
        if (c >= '0' && c <= '9')
            val = c - '0';
        else if (c >= 'A' && c <= 'F')
            val = 10 + c - 'A';
        else if (c >= 'a' && c <= 'f')
            val = 10 + c - 'a';
        else
            break;

        out <<= 4;
        out |= val;

        // fprint_hexnibble(val, stdout);

        c = *(str++);
        if (c >= '0' && c <= '9')
            val = c - '0';
        else if (c >= 'A' && c <= 'F')
            val = 10 + c - 'A';
        else if (c >= 'a' && c <= 'f')
            val = 10 + c - 'a';
        else
            break;

        out <<= 4;
        out |= val;

        // fprint_hexnibble(val, stdout);
    }
    // putc('\n', stdout);

    return out;
}

/// @param filepath Passed to `exec` syscall
/// @param args
///   NULL-terminated array of pointers to NULL-terminated strings.
///   Passed to `exec` syscall
void run_background_program(const char* const filepath, const char** args) {
    if (fork() == 0) syscall(SYS_exec, filepath, args);
}

#define IPC_KEYBOARD_MAGIC 0xf8
typedef struct ipc_keyboard_t {
    uint8_t magic;
    uint8_t is_pressed;
    uint16_t value;
} ipc_keyboard_t;

#define IPC_MOUSE_MAGIC 0xf9
typedef struct ipc_mouse_t {
    uint8_t magic;
    int32_t delta_x;
    int32_t delta_y;
    int32_t delta_scroll;
} ipc_mouse_t;

const uint32_t mouse_cursor_color = 0xffffffffu;
// In bits
#define MouseCursorWidth 16
// In bits
#define MouseCursorHeight 16
// This is a bitmap of the cursor, lmao.
// clang-format off
u8 mouse_cursor_bitmap[] = {
    0b10000000, 0b00000000,
    0b11000000, 0b00000000,
    0b11100000, 0b00000000,
    0b11110000, 0b00000000,
    0b11111000, 0b00000000,
    0b11111100, 0b00000000,
    0b11111110, 0b00000000,
    0b11111111, 0b00000000,
    0b11111111, 0b10000000,
    0b11111111, 0b11000000,
    0b11111111, 0b00000000,
    0b11111100, 0b00000000,
    0b11110000, 0b00000000,
    0b11000000, 0b00000000,
    0b00000000, 0b00000000,
    0b00000000, 0b00000000
};
// clang-format on
void draw_cursor(Framebuffer* fb, size_t cursor_x, size_t cursor_y) {
    clamp_draw_position(*fb, &cursor_x, &cursor_y);
    u32 size_x = MouseCursorWidth;
    u32 size_y = MouseCursorHeight;
    u32 initX = size_x;
    u32 diffX = fb->pixel_width - cursor_x;
    u32 diffY = fb->pixel_height - cursor_y;
    if (diffX < size_x) size_x = diffX;
    if (diffY < size_y) size_y = diffY;
    u32* pixel_ptr = (u32*)fb->base_address;
    for (u64 y = cursor_y; y < cursor_y + size_y; y++) {
        for (u64 x = cursor_x; x < cursor_x + size_x; x++) {
            s32 byte = ((x - cursor_x) + ((y - cursor_y) * initX)) / 8;
            if ((mouse_cursor_bitmap[byte] & (0b10000000 >> ((x - cursor_x) % 8))) > 0)
                *(u32*)(pixel_ptr + x + (y * fb->pixels_per_scanline)) = mouse_cursor_color;
        }
    }
}

int main(int argc, const char** argv) {
    // FIXME: Only do this when terminal is not graphical.
    // Set stdout unbuffered so the user can see updates as they type.
    // NOTE: Probably not very efficient for the terminal's output to be unbuffered.
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // TODO: If arguments are there, we should init framebuffer, draw to
    // it, etc. If it's not there, we should also be able to gracefully
    // handle that case.

    /*
    puts("Arguments:");
    for (int i = 0; i < argc; ++i) puts(argv[i]);
    fflush(NULL);
    */

    if (argc != 6) {
        printf("[INIT]: argc is not valid: %d\n", argc);
        return 1;
    }

    Framebuffer fb;
    fb.base_address = (void*)hexstring_to_number(argv[1]);
    fb.buffer_size = hexstring_to_number(argv[2]);
    fb.pixel_width = hexstring_to_number(argv[3]);
    fb.pixel_height = hexstring_to_number(argv[4]);
    fb.pixels_per_scanline = hexstring_to_number(argv[5]);
    // TODO: Pass format from kernel (which gets format passed from bootloader)
    fb.format = FB_FORMAT_DEFAULT;
    g_framebuffer = fb;

    // clear screen
    const uint32_t black = mkpixel(fb.format, 22, 23, 24, 0xff);
    fill_color(fb, black);

    // Allocate back buffer
    // TODO: flags
    void* back_buffer = (void*)syscall(SYS_map, NULL, fb.buffer_size, 0);
    if (!back_buffer) {
        printf("[INIT]: could not allocate graphical back buffer\n");
        return 1;
    }
    g_backbuffer = g_framebuffer;
    g_backbuffer.base_address = back_buffer;

    // Open GUI socket for listening
    int sockFD = sys_socket(0, 0, 0);
    sockaddr addr;
    addr.type = LENSOR16;
    const char socket_path[] = "!GUI";
    memset(addr.data, 0, SOCK_ADDR_MAX_SIZE);
    memcpy(addr.data, &socket_path, sizeof(socket_path) - 1);
    // bind (set our address)
    sys_bind(sockFD, &addr, sizeof(sockaddr));
    // listen (mark self as server)
    sys_listen(sockFD, 32);

    // Open an event queue to be notified when an incoming connection is
    // coming in on the server socket. This isn't really needed; we could
    // use the blocking mechanism of accept() for this. However, with this
    // technique, it would theoretically be possible to be doing other
    // things first (like handling all current connections with another
    // event queue; we'll get there) before checking if any incoming
    // connections have come in.
    int listen_queue = sys_kqueue();

    const size_t changelist_size = 4;
    Event changelist[changelist_size];
    memset(changelist, 0, sizeof(changelist));

    changelist[0].Type = EVENTTYPE_READY_TO_READ;
    changelist[0].Filter.ProcessFD = sockFD;
    changelist[0].Flags |= EVENTFLAGS_CHANGE_ADD_REMOVE;

    changelist[1].Type = EVENTTYPE_KEYBOARD;
    changelist[1].Filter.ProcessFD = -1;
    changelist[1].Flags |= EVENTFLAGS_CHANGE_ADD_REMOVE;

    changelist[2].Type = EVENTTYPE_MOUSE;
    changelist[2].Filter.ProcessFD = -1;
    changelist[2].Flags |= EVENTFLAGS_CHANGE_ADD_REMOVE;

    // This applies the above changes to the event queue, meaning we will
    // recieve events when the given file descriptor is ready to read from.
    // In the case of a local socket, that means a process has connected.
    sys_kevent(listen_queue, changelist, 3, NULL, 0);

    const size_t eventlist_size = 4;
    Event eventlist[eventlist_size];
    memset(eventlist, 0, sizeof(eventlist));

    // We can now detect a new client connection (ready to accept() without
    // blocking) via
    //     if (sys_kevent(listen_queue, NULL, 0, eventlist, eventlist_size) == 0)

    // Open terminal program in background
    const char* sh_args[1] = {NULL};
    run_background_program("/fs0/bin/term", sh_args);

    typedef struct window_t {
        void* shared_region;
        unsigned int x;
        unsigned int y;
        unsigned int width;
        unsigned int height;
        int shared_region_id;
        int client_fd;
        bool hidden;
    } window_t;

    window_t windows[8] = {0};

    typedef struct focus_t {
        window_t* window;

        ssize_t cursor_x;
        ssize_t cursor_y;

        bool left_control;
        bool left_shift;
        bool right_shift;
        bool left_alt;
        bool right_alt;
        bool left_super;
        bool right_super;
    } focus_t;

    focus_t focus;
    focus.window = &windows[0];
    focus.cursor_x = 0;
    focus.cursor_y = 0;

    while (true) {
        // Handle Incoming Requests on GUI Socket, Creating A New Window
        // FIXME: We may not handle all events, doing it like this.
        if (sys_kevent(listen_queue, NULL, 0, eventlist, eventlist_size) == 0) {
            // TODO: Handle all events in event list.
            if (eventlist[0].Type == EVENTTYPE_READY_TO_READ && eventlist[0].Filter.ProcessFD == sockFD) {
                printf("Got incoming connection...\n");

                sockaddr connected_addr;
                size_t connected_addrlen = sizeof(sockaddr);
                int clientFD = -1;
                // Attempt to accept incoming connection. If given the retry return code,
                // retry.
                do {
                    printf("[SERVE]: Accepting...\n");
                    fflush(stdout);
                    // We will block here until a connection is made.
                    clientFD = sys_accept(sockFD, &connected_addr, &connected_addrlen);
                    printf("[SERVE]: accept returned %d\n", clientFD);
                    fflush(stdout);
                } while (clientFD == -2);

                if (clientFD < 0) {
                    close(sockFD);
                    printf("[SERVE]: `accept` failed: %d\n", clientFD);
                    return 1;
                }

                window_t* window;
                for (int i = 0; i < sizeof(windows) / sizeof(window_t); ++i) {
                    window = &windows[i];
                    if (!window->shared_region) break;
                    window = nullptr;
                }
                if (!window) {
                    printf("[SERVE]: too many windows, ignoring request...\n");
                    continue;
                }

                uintptr_t* shared_data = NULL;
                int id = syscall(SYS_shared_memory_allocate, &shared_data, g_framebuffer.buffer_size);
                printf("id:%d data:%p\n", id, shared_data);

                // Book-keep shared_data pointer and id (create new window)
                window->shared_region = shared_data;
                window->shared_region_id = id;
                window->width = g_framebuffer.pixel_width;
                window->height = g_framebuffer.pixel_height;
                window->client_fd = clientFD;
                window->hidden = false;
                // TODO: Register change in kqueue to be notified when clientFD is
                // closed/EOF status. This is an "easy" way to tell when the process no
                // longer wants it's window, whether from no longer running or from
                // specifically requesting the window to be closed.

                // Communicate basic framebuffer data to client through shared memory.
                *shared_data++ = g_framebuffer.buffer_size;
                *shared_data++ = g_framebuffer.pixel_width;
                *shared_data++ = g_framebuffer.pixel_height;

                uintptr_t payload[3] = {69, 420, id};

                printf("[SERVE]: writing...\n");
                fflush(stdout);

                sys_write(
                    clientFD,
                    (uint8_t*)payload,
                    sizeof(payload),
                    LENSOROS_SYSCALL_WRITE_FLAG_NOBLOCK);
            }
            else if (eventlist[0].Type == EVENTTYPE_KEYBOARD) {
                EventData_KeyboardInput* e_data = (EventData_KeyboardInput*)&eventlist[0].Data;
                // printf("[SERVE]: Got keyboard input %d %d\n", e_data->press, e_data->value);

                if (e_data->value == LENSOR_KEY_LEFTCTRL) {
                    focus.left_control = e_data->press;
                }
                // TODO: right control
                else if (e_data->value == LENSOR_KEY_LEFTSHIFT) {
                    focus.left_shift = e_data->press;
                }
                else if (e_data->value == LENSOR_KEY_RIGHTSHIFT) {
                    focus.right_shift = e_data->press;
                }
                else if (e_data->value == LENSOR_KEY_LEFTALT) {
                    focus.left_alt = e_data->press;
                }
                else if (e_data->value == LENSOR_KEY_MOUSE_LEFT) {
                    // TODO: If mouse click is over window stack, calculate if it's over an
                    // open window selector; if it is, focus that window. Also move it in Z
                    // ordering.
                }
                // TODO: right alt
                // TODO: left/right super
                else if (focus.window && focus.window->shared_region) {
                    ipc_keyboard_t keyboard_message;
                    keyboard_message.magic = IPC_KEYBOARD_MAGIC;
                    keyboard_message.value = e_data->value;
                    keyboard_message.is_pressed = e_data->press;
                    // TODO: non-blocking, in case our GUI program isn't reading from the
                    // socket.
                    // We should write this event to a ring buffer, then, we should only write
                    // to the client FD once it is actually writable.
                    // write(focus.window->client_fd, &keyboard_message, sizeof(keyboard_message));
                }
            }
            else if (eventlist[0].Type == EVENTTYPE_MOUSE) {
                EventData_MouseInput* e_data = (EventData_MouseInput*)&eventlist[0].Data;
                // printf("[SERVE]: Got mouse input (%d, %d)\n", e_data->delta_x, e_data->delta_y);
                focus.cursor_x += e_data->delta_x;
                focus.cursor_y += e_data->delta_y;

                if (focus.cursor_x < 0) focus.cursor_x = 0;
                if (focus.cursor_x >= g_framebuffer.pixel_width)
                    focus.cursor_x = g_framebuffer.pixel_width - 1;

                if (focus.cursor_y < 0) focus.cursor_y = 0;
                if (focus.cursor_y >= g_framebuffer.pixel_height)
                    focus.cursor_y = g_framebuffer.pixel_height - 1;

                if (focus.window && focus.window->shared_region) {
                    ipc_mouse_t mouse_message;
                    mouse_message.magic = IPC_MOUSE_MAGIC;
                    mouse_message.delta_x = e_data->delta_x;
                    mouse_message.delta_y = e_data->delta_y;
                    mouse_message.delta_scroll = e_data->wheel_delta;
                    sys_write(
                        focus.window->client_fd,
                        (uint8_t*)&mouse_message,
                        sizeof(mouse_message),
                        LENSOROS_SYSCALL_WRITE_FLAG_NOBLOCK);
                }
            }
            else {
                printf("[SERVE]: Unhandled kqueue event\n");
            }
        }

        // Draw Each Window's Framebuffer to the Actual Framebuffer
        for (int i = 0; i < sizeof(windows) / sizeof(window_t); ++i) {
            const window_t* window = &windows[i];
            if (!window->shared_region) continue;

            // Define pixel size (TODO: get from kernel)
            const int bytes_per_pixel = 4;

            // Get screen dimensions and pitches
            const int screen_pitch = g_framebuffer.pixel_width * bytes_per_pixel;
            const int window_pitch = window->width * bytes_per_pixel;

            // Cast to uint8_t* for byte-level pointer arithmetic
            const uint8_t* screen_fb = (uint8_t*)g_backbuffer.base_address;
            const uint8_t* window_fb = (uint8_t*)window->shared_region;

            // Clip the window boundaries to prevent drawing off-screen (kernel panics/segfaults)
            const int start_y = window->y;
            const int end_y
                = (window->y + window->height > g_framebuffer.pixel_height)
                      ? g_framebuffer.pixel_height
                      : (window->y + window->height);

            const int start_x = window->x;
            const int end_x
                = (window->x + window->width > g_framebuffer.pixel_width)
                      ? g_framebuffer.pixel_width
                      : (window->x + window->width);

            // Calculate dimensions to actually copy after clipping
            const int copy_width_pixels = end_x - start_x;
            if (copy_width_pixels <= 0) continue;

            // Loop through each visible row of the window
            for (int y = start_y; y < end_y; ++y) {
                // Find where this window row starts relative to the window's own buffer
                const int win_local_y = y - window->y;
                const int win_local_x = start_x - window->x;
                const uint8_t* src_row = window_fb + (win_local_y * window_pitch) + (win_local_x * bytes_per_pixel);

                // Find the matching row on the physical screen
                const uint8_t* dest_row = screen_fb + (y * screen_pitch) + (start_x * bytes_per_pixel);

                // Copy exactly one row segment
                memcpy(dest_row, src_row, copy_width_pixels * bytes_per_pixel);
            }
        }

        // TODO: Draw Taskbar/Window Stack
        const uint32_t window_stack_height = 28;
        uint32_t window_stack_begin_y = g_framebuffer.pixel_height - window_stack_height;

        // [WINSTACK]: Draw background
        fill_rect(
            g_backbuffer,
            black,
            0,
            window_stack_begin_y,
            g_framebuffer.pixel_width,
            window_stack_height);
        // [WINSTACK]: Delineating Stripe
        const uint32_t orange = mkpixel(g_framebuffer.format, 0xff, 0x62, 0x00, 0xff);
        fill_rect(
            g_backbuffer,
            orange,
            0,
            window_stack_begin_y,
            g_framebuffer.pixel_width,
            window_stack_height / 8);

        for (int i = 0; i < sizeof(windows) / sizeof(window_t); ++i) {
            const window_t* window = &windows[i];
            if (!window->shared_region) continue;

            const uint32_t present_window_color = mkpixel(g_framebuffer.format, 0xff, 0xff, 0xff, 0xff);
            const uint32_t hidden_window_color = mkpixel(g_framebuffer.format, 0x67, 0x67, 0x67, 0xff);
            const uint32_t focused_window_color = orange;
            uint32_t color = present_window_color;
            if (window == focus.window) {
                color = focused_window_color;
            }
            else if (window->hidden) {
                color = hidden_window_color;
            }

            const uint32_t window_selector_width = 16;
            fill_rect(
                g_backbuffer,
                color,
                i * window_selector_width,
                window_stack_begin_y,
                window_selector_width,
                window_stack_height);
        }

        // Draw Mouse Cursor
        draw_cursor(&g_backbuffer, focus.cursor_x, focus.cursor_y);

        // Swap Back Buffer <-> Front Buffer
        memcpy(
            g_framebuffer.base_address,
            g_backbuffer.base_address,
            g_framebuffer.buffer_size);

        // Yield
        syscall(SYS_cooperative_yield);
    }

    close(sockFD);

    return 0;
}
