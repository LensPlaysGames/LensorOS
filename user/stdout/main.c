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
#include <lensor/ipc.h>
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

const uint32_t window_stack_height = 28;
const uint32_t window_selector_width = 27;
const uint32_t window_selector_separator_width = 1;

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

typedef struct window_t {
    // pointer to shared memory region between client process and the
    // compositor, us.
    void* shared_region;
    // x-axis coordinate of top-left of window.
    // where the window begins from the left.
    unsigned int x;
    // y-axis coordinate of top-left of window.
    // where the window begins from the top.
    unsigned int y;
    // visible window width
    // how far the window draws past it's x-axis coordinate position.
    unsigned int width;
    // visible window height
    // how far the window draws past it's y-axis coordinate position.
    unsigned int height;
    // Shared memory region ID we negotiated with the client.
    int shared_region_id;
    // How we talk to the client.
    int client_fd;
    // true:  window's canvas is not painted.
    // false: window's canvas is painted according to Z-value.
    // default: false
    bool hidden;
    // true:  window recieves mouse events in the form of ipc_mouse_delta_t.
    // false: window recieves mouse events in the form of ipc_mouse_postion_t.
    // default: false
    bool mouse_delta;
} window_t;

static inline bool window_valid(const window_t* window) {
    return window && window->shared_region;
}

static inline bool point_within_window(
    const window_t* window,
    const ssize_t x,
    const ssize_t y) {
    return window
           && x >= window->x && x < window->x + window->width
           && y >= window->y && y < window->y + window->height;
}

typedef struct focus_t {
    window_t* window;

    ssize_t cursor_x;
    ssize_t cursor_y;

    bool left_control;
    bool right_control;
    bool left_shift;
    bool right_shift;
    bool left_alt;
    bool right_alt;
    bool left_super;
    bool right_super;
} focus_t;

#define MAX_WINDOW_COUNT 8
typedef struct CompositorContext {
    window_t windows[MAX_WINDOW_COUNT];
    focus_t focus;

    ProcFD incoming_client_socket;
} CompositorContext;

void handle_event_incoming_client(Event incoming_client_event, CompositorContext* context) {
    if (context == NULL) return;

    // EventData_ReadyToReadWrite* readwrite_data = (EventData_ReadyToReadWrite*)&incoming_client_event.Data[0];

    printf("[INIT]: Got incoming connection...\n");

    ProcFD incoming_client_fd = context->incoming_client_socket;

    sockaddr connected_addr;
    size_t connected_addrlen = sizeof(sockaddr);
    int clientFD = -1;
    // Attempt to accept incoming connection. If given the retry return code,
    // retry.
    do {
        // printf("[INIT]: Accepting incoming client connection...\n");
        fflush(stdout);
        // We will block here until a connection is made.
        clientFD = sys_accept(
            context->incoming_client_socket,
            &connected_addr,
            &connected_addrlen);
        // printf("[INIT]: accept returned %d\n", clientFD);
        fflush(stdout);
    } while (clientFD == -2);

    if (clientFD < 0) {
        close(incoming_client_fd);
        printf("[INIT]: `accept` failed: %d\n", clientFD);
        return;
    }

    window_t* window;
    for (int i = 0; i < sizeof(context->windows) / sizeof(context->windows[0]); ++i) {
        window = &context->windows[i];
        if (!window->shared_region) break;
        window = NULL;
    }
    if (!window) {
        printf("[INIT]: too many windows, ignoring request...\n");
        return;
    }

    uintptr_t* shared_data = NULL;
    int id = syscall(SYS_shared_memory_allocate, &shared_data, g_framebuffer.buffer_size);
    printf("[INIT]: shmem -- id:%d data:%p\n", id, shared_data);

    // Book-keep shared_data pointer and id (create new window)
    window->shared_region = shared_data;
    window->shared_region_id = id;
    window->width = g_framebuffer.pixel_width;
    window->height = g_framebuffer.pixel_height;
    window->client_fd = clientFD;
    window->hidden = false;

    // If no windows are open, automatically focus the first opened window.
    if (context->focus.window == NULL)
        context->focus.window = window;

    // TODO: Register change in kqueue to be notified when clientFD is
    // closed/EOF status. This is an "easy" way to tell when the process no
    // longer wants it's window, whether from no longer running or from
    // specifically requesting the window to be closed.

    // Communicate basic framebuffer data to client through shared memory.
    *shared_data++ = g_framebuffer.buffer_size;
    *shared_data++ = g_framebuffer.pixel_width;
    *shared_data++ = g_framebuffer.pixel_height;

    uintptr_t payload[3] = {69, 420, id};

    printf("[INIT]: writing payload...\n");
    fflush(stdout);

    sys_write(
        clientFD,
        (uint8_t*)payload,
        sizeof(payload),
        LENSOROS_SYSCALL_WRITE_FLAG_NOBLOCK);
}

void handle_event_keyboard(Event event, CompositorContext* context) {
    EventData_KeyboardInput* keyboard_data = (EventData_KeyboardInput*)&event.Data[0];
    // printf("[SERVE]: Got keyboard input %d %u\n", keyboard_data->press, keyboard_data->value);

    switch (keyboard_data->value) {
        case LENSOR_KEY_LEFTCTRL:
            context->focus.left_control = keyboard_data->press;
            break;
        case LENSOR_KEY_RIGHTCTRL:
            context->focus.right_control = keyboard_data->press;
            break;
        case LENSOR_KEY_LEFTSHIFT:
            context->focus.left_shift = keyboard_data->press;
            break;
        case LENSOR_KEY_RIGHTSHIFT:
            context->focus.right_shift = keyboard_data->press;
            break;
        case LENSOR_KEY_LEFTALT:
            context->focus.left_alt = keyboard_data->press;
            break;
        case LENSOR_KEY_RIGHTALT:
            context->focus.right_alt = keyboard_data->press;
            break;
        case LENSOR_KEY_LEFTSUPER:
            context->focus.left_super = keyboard_data->press;
            break;
        case LENSOR_KEY_RIGHTSUPER:
            context->focus.right_super = keyboard_data->press;
            break;
        case LENSOR_KEY_MOUSE_LEFT: {
            // If mouse click is over window stack, calculate if it's over an
            // open window selector; if it is, focus that window. Also move it in Z
            // ordering.
            uint32_t window_stack_begin_y = g_framebuffer.pixel_height - window_stack_height;
            if (context->focus.cursor_y >= window_stack_begin_y) {
                const uint32_t window_stack_index
                    = context->focus.cursor_x / (window_selector_width + window_selector_separator_width);

                const uint32_t window_count = (sizeof(context->windows) / sizeof(context->windows[0]));
                if (window_stack_index < window_count) {
                    window_t clicked_window = context->windows[window_stack_index];
                    if (clicked_window.shared_region) {
                        clicked_window.hidden = false;
                        // shift all windows before clicked window forward
                        //   v
                        // A B C D -> _ A C D
                        memmove(
                            &context->windows[1],
                            &context->windows[0],
                            window_stack_index * sizeof(context->windows[0]));
                        // move clicked window to front
                        // B A C D
                        context->windows[0] = clicked_window;
                        context->focus.window = &context->windows[0];
                    }
                }
            }
        } break;

        case LENSOR_KEY_Q:
            if (keyboard_data->press && context->focus.left_alt) {
                printf("[SERVE]: got SUPER+Q, closing focused window\n");
                const uintptr_t window_index = context->focus.window - &context->windows[0];
                const uint32_t window_count = (sizeof(context->windows) / sizeof(context->windows[0]));
                // Close focused window
                window_t* window = context->focus.window;
                // do not draw it.
                window->hidden = true;
                window->shared_region = NULL;

                if (window_index + 1 < window_count) {
                    //   v
                    // A B C D -> A C D
                    memmove(
                        &context->windows[window_index],
                        &context->windows[window_index + 1],
                        sizeof(context->windows[0]) * (window_count - 1 - window_index));
                }
                memset(
                    &context->windows[window_count - 1],
                    0,
                    sizeof(context->windows[0]));

                if (window_valid(&context->windows[0]))
                    context->focus.window = &context->windows[0];
                else
                    context->focus.window = NULL;

                // release from shared memory region
                syscall(SYS_shared_memory_release, window->shared_region_id);
                // close (our side of) client file descriptor
                close(window->client_fd);
                // TODO: unregister kqueue listening for clientFD; or, we could
                // alternatively listen for a close/EOF event and unregister
                // automatically.
            }
            break;
    }

    if (context->focus.window && context->focus.window->shared_region) {
        ipc_keyboard_t keyboard_message;
        keyboard_message.magic = IPC_KEYBOARD_MAGIC;
        keyboard_message.value = keyboard_data->value;
        keyboard_message.is_pressed = keyboard_data->press;
        // TODO: We should write this event to a ring buffer, then, we should only write
        // to the client FD once it is actually writable.
        sys_write(
            context->focus.window->client_fd,
            (uint8_t*)&keyboard_message,
            sizeof(keyboard_message),
            LENSOROS_SYSCALL_WRITE_FLAG_NOBLOCK);
    }
}

void handle_event_mouse(Event event, CompositorContext* context) {
    EventData_MouseInput* mouse_data = (EventData_MouseInput*)&event.Data[0];

    // printf("[SERVE]: Got mouse input (%d, %d)\n", mouse_data->delta_x, mouse_data->delta_y);
    context->focus.cursor_x += mouse_data->delta_x;
    context->focus.cursor_y += mouse_data->delta_y;

    if (context->focus.cursor_x < 0) context->focus.cursor_x = 0;
    if (context->focus.cursor_x >= g_framebuffer.pixel_width)
        context->focus.cursor_x = g_framebuffer.pixel_width - 1;

    if (context->focus.cursor_y < 0) context->focus.cursor_y = 0;
    if (context->focus.cursor_y >= g_framebuffer.pixel_height)
        context->focus.cursor_y = g_framebuffer.pixel_height - 1;

    const window_t* window = context->focus.window;
    const ssize_t cursor_x = context->focus.cursor_x;
    const ssize_t cursor_y = context->focus.cursor_y;
    if (window_valid(window)
        && !window->hidden
        && point_within_window(window, cursor_x, cursor_y)) {
        if (window->mouse_delta) {
            ipc_mouse_delta_t mouse_message;
            mouse_message.magic = IPC_MOUSE_DELTA_MAGIC;
            mouse_message.delta_x = mouse_data->delta_x;
            mouse_message.delta_y = mouse_data->delta_y;
            // TODO: RING BUFFER
            sys_write(
                context->focus.window->client_fd,
                (uint8_t*)&mouse_message,
                sizeof(mouse_message),
                LENSOROS_SYSCALL_WRITE_FLAG_NOBLOCK);
        }
        else {
            ipc_mouse_position_t mouse_message;
            mouse_message.magic = IPC_MOUSE_POSITION_MAGIC;
            mouse_message.local_x = cursor_x;
            mouse_message.local_y = cursor_y;
            // TODO: ring buffer, yada yada
            sys_write(
                window->client_fd,
                (uint8_t*)&mouse_message,
                sizeof(mouse_message),
                LENSOROS_SYSCALL_WRITE_FLAG_NOBLOCK);
        }
    }
}

void handle_event(Event event, CompositorContext* context) {
    if (event.Type == EVENTTYPE_READY_TO_READ
        && event.Filter.ProcessFD == context->incoming_client_socket)
        handle_event_incoming_client(event, context);

    else if (event.Type == EVENTTYPE_KEYBOARD)
        handle_event_keyboard(event, context);

    else if (event.Type == EVENTTYPE_MOUSE)
        handle_event_mouse(event, context);

    else
        printf("[SERVE]: Unhandled kqueue event (type %u)\n", event.Type);
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

    CompositorContext context = {0};
    context.incoming_client_socket = sockFD;

    if (context.incoming_client_socket == 0) {
        printf("[INIT]:ERROR: Init process internal error: did not set incoming client socket, no windows will be able to open\n");
        return 1;
    }

    const size_t eventlist_size = 4;
    Event eventlist[eventlist_size];
    memset(eventlist, 0, sizeof(eventlist));

    // We can now detect a new client connection (ready to accept() without
    // blocking) via
    //     if (sys_kevent(listen_queue, NULL, 0, eventlist, eventlist_size) == 0)

    // Open terminal program in background
    const char* sh_args[1] = {NULL};
    run_background_program("/fs0/bin/term", sh_args);

    while (true) {
        // If any event that we've registered to listen for has occurred, the
        // kernel will have stored them in our event queue. Either pop the events
        // out of the event queue, or block until one is ready to handle.
        if (sys_kevent(listen_queue, NULL, 0, eventlist, eventlist_size) == 0) {
            // Handle all valid events in event list.
            for (int i = 0; i < eventlist_size; ++i) {
                if (eventlist[i].Type == EVENTTYPE_INVALID)
                    break;

                handle_event(eventlist[i], &context);
                memset(&eventlist[i], 0, sizeof(eventlist[i]));
            }
        }

        // Draw Each Window's Framebuffer to the Actual Framebuffer
        for (int i = sizeof(context.windows) / sizeof(context.windows[0]); i; --i) {
            const window_t* window = &context.windows[i - 1];
            if (!window->shared_region) continue;

            // Define pixel size (TODO: get from kernel)
            const int bytes_per_pixel = 4;

            // Get screen dimensions and pitches
            const int screen_pitch = g_framebuffer.pixel_width * bytes_per_pixel;
            const int window_pitch = window->width * bytes_per_pixel;

            // Cast to uint8_t* for byte-level pointer arithmetic
            uint8_t* screen_fb = (uint8_t*)g_backbuffer.base_address;
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
                uint8_t* dest_row = screen_fb + (y * screen_pitch) + (start_x * bytes_per_pixel);

                // Copy exactly one row segment
                memcpy(dest_row, src_row, copy_width_pixels * bytes_per_pixel);
            }
        }

        // Draw Taskbar/Window Stack
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

        int selector_count = 0;
        for (int i = sizeof(context.windows) / sizeof(context.windows[0]); i; --i) {
            const window_t* window = &context.windows[i - 1];
            if (!window->shared_region) continue;

            const uint32_t present_window_color = mkpixel(g_framebuffer.format, 0xff + window->shared_region_id * 0x10, 0xff, 0xff, 0xff);
            const uint32_t hidden_window_color = mkpixel(g_framebuffer.format, 0x67, 0x67, 0x67, 0xff);
            const uint32_t focused_window_color = orange;
            uint32_t color = present_window_color;
            if (window == context.focus.window) {
                color = focused_window_color;
            }
            else if (window->hidden) {
                color = hidden_window_color;
            }

            fill_rect(
                g_backbuffer,
                color,
                selector_count * window_selector_width
                    + selector_count * window_selector_separator_width,
                window_stack_begin_y,
                window_selector_width,
                window_stack_height);

            ++selector_count;
        }

        // Draw Mouse Cursor
        draw_cursor(&g_backbuffer, context.focus.cursor_x, context.focus.cursor_y);

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
