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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscalls.h>
#include <sys/types.h>
#include <unistd.h>

// This is a hack and should be removed
static Framebuffer g_framebuffer;

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
    // This applies the above change to the event queue, meaning we will
    // recieve events when the given file descriptor is ready to read from.
    // In the case of a local socket, that means a process has connected.
    sys_kevent(listen_queue, changelist, 1, NULL, 0);

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
        unsigned int z;
        unsigned int width;
        unsigned int height;
        int shared_region_id;
        int client_fd;
    } window_t;

    window_t windows[8] = {0};

    while (true) {
        // Handle Incoming Requests on GUI Socket, Creating A New Window
        // FIXME: We may not handle all events, doing it like this.
        if (sys_kevent(listen_queue, NULL, 0, eventlist, eventlist_size) == 0) {
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
            // TODO: Register change in (a new) kqueue to be notified when clientFD is
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

            write(clientFD, payload, sizeof(payload));
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
            // TODO: use a back buffer
            const uint8_t* screen_fb = (uint8_t*)back_buffer;
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

        // Swap Back Buffer <-> Front Buffer
        memcpy(g_framebuffer.base_address, back_buffer, g_framebuffer.buffer_size);

        // Yield
        syscall(SYS_cooperative_yield);
    }

    close(sockFD);

    return 0;
}
