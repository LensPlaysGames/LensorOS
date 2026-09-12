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

#include <ctype.h>
#include <framebuffer.h>
#include <ints.h>
#include <lensor/ipc.h>
#include <lensor/keys.h>
#include <psf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscalls.h>
#include <sys/types.h>
#include <unistd.h>

// FIXME: This is a hack and should be removed
static Framebuffer g_framebuffer;
static PSF1_FONT g_font;

static uint8_t simple_keymap[LENSOR_KEY_KPDOT + 1] = {
    '\0',
    '\0',  // LENSOR_KEY_ESC
    '1',   // LENSOR_KEY_DIGIT1
    '2',   // LENSOR_KEY_DIGIT2
    '3',   // LENSOR_KEY_DIGIT3
    '4',   // LENSOR_KEY_DIGIT4
    '5',   // LENSOR_KEY_DIGIT5
    '6',   // LENSOR_KEY_DIGIT6
    '7',   // LENSOR_KEY_DIGIT7
    '8',   // LENSOR_KEY_DIGIT8
    '9',   // LENSOR_KEY_DIGIT9
    '0',   // LENSOR_KEY_DIGIT0
    '-',   // LENSOR_KEY_MINUS
    '=',   // LENSOR_KEY_EQUAL
    '\b',  // LENSOR_KEY_BACKSPACE
    '\t',  // LENSOR_KEY_TAB
    'q',   // LENSOR_KEY_Q
    'w',   // LENSOR_KEY_W
    'e',   // LENSOR_KEY_E
    'r',   // LENSOR_KEY_R
    't',   // LENSOR_KEY_T
    'y',   // LENSOR_KEY_Y
    'u',   // LENSOR_KEY_U
    'i',   // LENSOR_KEY_I
    'o',   // LENSOR_KEY_O
    'p',   // LENSOR_KEY_P
    '[',   // LENSOR_KEY_LEFTBRACE
    ']',   // LENSOR_KEY_RIGHTBRACE
    '\n',  // LENSOR_KEY_ENTER
    '\0',  // LENSOR_KEY_LEFTCTRL
    'a',   // LENSOR_KEY_A
    's',   // LENSOR_KEY_S
    'd',   // LENSOR_KEY_D
    'f',   // LENSOR_KEY_F
    'g',   // LENSOR_KEY_G
    'h',   // LENSOR_KEY_H
    'j',   // LENSOR_KEY_J
    'k',   // LENSOR_KEY_K
    'l',   // LENSOR_KEY_L
    ';',   // LENSOR_KEY_SEMICOLON
    '\'',  // LENSOR_KEY_APOSTROPHE
    '`',   // LENSOR_KEY_GRAVE
    '\0',  // LENSOR_KEY_LEFTSHIFT
    '\\',  // LENSOR_KEY_BACKSLASH
    'z',   // LENSOR_KEY_Z
    'x',   // LENSOR_KEY_X
    'c',   // LENSOR_KEY_C
    'v',   // LENSOR_KEY_V
    'b',   // LENSOR_KEY_B
    'n',   // LENSOR_KEY_N
    'm',   // LENSOR_KEY_M
    ',',   // LENSOR_KEY_COMMA
    '.',   // LENSOR_KEY_DOT
    '/',   // LENSOR_KEY_SLASH
    '\0',  // LENSOR_KEY_RIGHTSHIFT
    '*',   // LENSOR_KEY_KPASTERISK
    '\0',  // LENSOR_KEY_LEFTALT
    ' ',   // LENSOR_KEY_SPACE
    '\0',  // LENSOR_KEY_CAPSLOCK
    '\0',  // LENSOR_KEY_F1
    '\0',  // LENSOR_KEY_F2
    '\0',  // LENSOR_KEY_F3
    '\0',  // LENSOR_KEY_F4
    '\0',  // LENSOR_KEY_F5
    '\0',  // LENSOR_KEY_F6
    '\0',  // LENSOR_KEY_F7
    '\0',  // LENSOR_KEY_F8
    '\0',  // LENSOR_KEY_F9
    '\0',  // LENSOR_KEY_F10
    '\0',  // LENSOR_KEY_NUMLOCK
    '\0',  // LENSOR_KEY_SCROLLLOCK
    '7',   // LENSOR_KEY_KP7
    '8',   // LENSOR_KEY_KP8
    '9',   // LENSOR_KEY_KP9
    '-',   // LENSOR_KEY_KPMINUS
    '4',   // LENSOR_KEY_KP4
    '5',   // LENSOR_KEY_KP5
    '6',   // LENSOR_KEY_KP6
    '+',   // LENSOR_KEY_KPPLUS
    '1',   // LENSOR_KEY_KP1
    '2',   // LENSOR_KEY_KP2
    '3',   // LENSOR_KEY_KP3
    '0',   // LENSOR_KEY_KP0
    '.'    // LENSOR_KEY_KPDOT
};

char to_capital(char c) {
    if (islower(c)) return c - ('a' - 'A');

    const char* lower = "1234567890,./;'\\-";
    const char* upper = "!@#$%^&*()<>?:\"|_";
    const char* found = strchr(lower, c);
    if (found) return upper[found - lower];

    return c;
}

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

void draw_psf1_char(const Framebuffer fb, const PSF1_FONT font, size_t position_x, size_t position_y, const u8 c) {
    const u32 fg_color = mkpixel(fb.format, 0xff, 0xff, 0xff, 0xff);
    const u32 bg_color = mkpixel(fb.format, 22, 23, 24, 0xff);

    clamp_draw_position(fb, &position_x, &position_y);

    usz size_x = psf1_width(font);
    const usz initX = size_x;
    usz size_y = font.header.character_size;
    usz diffX = fb.pixel_width - position_x;
    usz diffY = fb.pixel_height - position_y;
    if (diffX < size_x) size_x = diffX;
    if (diffY < size_y) size_y = diffY;

    u8* bitmap = psf1_char_bitmap(font, c);
    u32* pixel_ptr = (u32*)fb.base_address;

    for (usz y = position_y; y < position_y + size_y; ++y) {
        for (usz x = position_x; x < position_x + size_x; ++x) {
            u32 color = bg_color;

            usz byte = ((x - position_x) + ((y - position_y) * initX)) / 8;
            if ((bitmap[byte] & (0b10000000 >> ((x - position_x) % 8))) > 0)
                color = fg_color;

            write_pixel(fb, color, x, y);
        }
    }
}

static void draw_psf1_cr(const Framebuffer fb, const PSF1_FONT font, size_t* x) {
    *x = 0;
}
static void draw_psf1_lf(const Framebuffer fb, const PSF1_FONT font, size_t* y) {
    *y += font.header.character_size;
}
static void draw_psf1_crlf(const Framebuffer fb, const PSF1_FONT font, size_t* x, size_t* y) {
    // TODO: use bg color
    fill_rect(fb, mkpixel(fb.format, 22, 23, 24, 255), *x, *y, fb.pixel_width, psf1_height(font));
    draw_psf1_cr(fb, font, x);
    draw_psf1_lf(fb, font, y);
}

void draw_psf1_string_view(Framebuffer fb, const PSF1_FONT font, size_t* x, size_t* y, const char* str, size_t length) {
    char c;
    while ((c = *str++) && length--) {
        if (c == '\n')
            draw_psf1_crlf(fb, font, x, y);
        else if (c == '\r')
            *x = 0;
        else if (c == '\b')
            *x -= psf1_width(font);
        else {
            draw_psf1_char(fb, font, *x, *y, c);
            *x += psf1_width(font);
        }
    }
}

void draw_psf1_string(Framebuffer fb, const PSF1_FONT font, size_t* x, size_t* y, const char* str) {
    char c;
    while ((c = *str++)) {
        if (c == '\n')
            draw_psf1_crlf(fb, font, x, y);
        else if (c == '\r')
            *x = 0;
        else if (c == '\b')
            *x -= psf1_width(font);
        else {
            draw_psf1_char(fb, font, *x, *y, c);
            *x += psf1_width(font);
        }
    }
}

void draw_psf1_int(Framebuffer fb, const PSF1_FONT font, size_t* x, size_t* y, int val) {
    char numstr[32];
    memset(numstr, 0, 32);
    sprintf(numstr, "%d", val);
    draw_psf1_string(fb, font, x, y, numstr);
}

typedef struct Cursor {
    size_t x;
    size_t y;
} Cursor;
typedef struct CharacterBuffer {
    uint32_t* data;
    Cursor cursor;
    size_t width;
    size_t height;
} CharacterBuffer;

// width and height in amount of characters.
CharacterBuffer charbuf_create(size_t width, size_t height) {
    CharacterBuffer out = {};
    out.width = width;
    out.height = height;

    out.cursor.x = 0;
    out.cursor.y = 0;

    // TODO: handle allocation failure
    out.data = malloc(width * height + 1);

    return out;
}
void charbuf_delete(CharacterBuffer charbuf) {
    if (charbuf.data)
        free(charbuf.data);

    charbuf.data = NULL;
}
uint32_t* charbuf_at(const CharacterBuffer* charbuf, const size_t x, const size_t y) {
    return &charbuf->data[charbuf->width * y + x];
}
void charbuf_write(const CharacterBuffer* charbuf, const size_t x, const size_t y, const uint32_t c) {
    if (x >= charbuf->width) {
        puts("[term]: x out of bounds! refusing character buffer write");
        return;
    }
    if (y >= charbuf->height) {
        puts("[term]: y out of bounds! refusing character buffer write");
        return;
    }
    *charbuf_at(charbuf, x, y) = c;
}
void charbuf_putc(CharacterBuffer* charbuf, const uint32_t c) {
    if (c == '\b') {
        if (charbuf->cursor.x) {
            // Decrement cursor position
            charbuf->cursor.x -= 1;
            // Draw space over current cursor position
            charbuf_putc(charbuf, ' ');
            // Decrement cursor position (undo increment from writing space)
            charbuf->cursor.x -= 1;
        }
        else if (!charbuf->cursor.y) {
            // Backspace at very beginning of character buffer (replace first character with space).
            charbuf_putc(charbuf, ' ');
            charbuf->cursor.x = 0;
        }
        else {
            // TODO: At beginning of line, move cursor to end of last line.
            puts("\n[TERM]: TODO: backspace newline\n");
        }
    }
    else if (c == '\r') {
        charbuf->cursor.x = 0;
    }
    else if (c == '\n') {
        charbuf->cursor.y += 1;
        charbuf->cursor.x = 0;
        if (charbuf->cursor.y >= charbuf->height) {
            // FIXME: Hack for now to scroll entire character buffer.
            charbuf->cursor.y = 0;
            const uint32_t black = mkpixel(g_framebuffer.format, 22, 23, 24, 0xff);
            fill_color(g_framebuffer, black);
        }
        // FIXME: We probably have to draw spaces to the rest of the line in the
        // character buffer.
    }
    else {
        // Write character into character buffer.
        charbuf_write(charbuf, charbuf->cursor.x, charbuf->cursor.y, c);

        // Draw graphical character into framebuffer at x, y position
        draw_psf1_char(g_framebuffer, g_font, charbuf->cursor.x * psf1_width(g_font), charbuf->cursor.y * psf1_height(g_font), c);

        // Update cursor position.
        charbuf->cursor.x += 1;
    }
}
void charbuf_puts(CharacterBuffer* charbuf, const uint32_t* s) {
    if (!s) return;
    while (*s)
        charbuf_putc(charbuf, *s);
}

/// @param filepath Passed to `exec` syscall
/// @param args
///   NULL-terminated array of pointers to NULL-terminated strings.
///   Passed to `exec` syscall
void run_program_waitpid(ProcFD clientFD, const char* const filepath, const char** args, CharacterBuffer* charbuf) {
    const uint PIPE_END_READ = 0;
    const uint PIPE_END_WRITE = 1;

    u64 command_output_pipe[2] = {-1, -1};
    syscall(SYS_pipe, command_output_pipe);

    u64 command_input_pipe[2] = {-1, -1};
    syscall(SYS_pipe, command_input_pipe);

    // TODO: We want to
    // - Read output from command via "command output" pipe.
    // - Display command output.
    // - *new* Read ipc messages from our GUI client socket file descriptor.
    // - *new* Decode keyboard ipc messages, write them to "command input" pipe.
    // Requires opening command_input_pipe, wiring stdin of child to the read
    // end of it.

    pid_t cpid = fork();
    if (cpid) {
        // Close read end of command input pipe
        close(command_input_pipe[PIPE_END_READ]);
        // Close write end of command output pipe
        close(command_output_pipe[PIPE_END_WRITE]);

        // TODO: kqueue listening for:
        // - clientFD ready to read from
        // - command output pipe ready to read from

        bool do_capital = false;

        char c = 0;
        ssize_t bytes_read = 0;
        while ((bytes_read = sys_read(command_output_pipe[0], &c, 1, LENSOROS_SYSCALL_READ_FLAG_NOBLOCK))) {
            if (bytes_read > 0) {
                // Draw output to stdout (probably DbgOutDriver, AKA UART).
                putc(c, stdout);
                charbuf_putc(charbuf, c);
            }

            // Read from gui client socket file descriptor for events. Handle events.
            uint8_t ipc_buffer[256];
            ssize_t ipc_bytes_read = sys_read(
                clientFD,
                &ipc_buffer[0],
                sizeof(ipc_buffer),
                LENSOROS_SYSCALL_READ_FLAG_NOBLOCK);

            if (ipc_bytes_read <= 0) {
                syscall(SYS_cooperative_yield);
                continue;
            }

            // TODO: Handle multiple messages, if necessary.
            uint8_t magic = ipc_buffer[0];
            switch (magic) {
                case IPC_KEYBOARD_MAGIC: {
                    ipc_keyboard_t* keyboard_ipc = (ipc_keyboard_t*)&ipc_buffer[0];

                    if (keyboard_ipc->value == LENSOR_KEY_LEFTSHIFT
                        || keyboard_ipc->value == LENSOR_KEY_RIGHTSHIFT)
                        do_capital = keyboard_ipc->is_pressed;
                    else if (keyboard_ipc->value == LENSOR_KEY_CAPSLOCK && keyboard_ipc->is_pressed)
                        do_capital = !do_capital;

                    // Ignore key releases
                    if (!keyboard_ipc->is_pressed) break;

                    // Translate LENSOR_KEY_* value to UTF8 bytes we can write to the running
                    // command.
                    u8 typed_char = 0;
                    if (keyboard_ipc->value < (sizeof(simple_keymap) / sizeof(simple_keymap[0])))
                        typed_char = simple_keymap[keyboard_ipc->value];

                    if (do_capital)
                        typed_char = to_capital(typed_char);

                    // Write keypresses to write end of command input pipe
                    if (typed_char)
                        write(command_input_pipe[PIPE_END_WRITE], &typed_char, 1);
                } break;

                default:
                    break;
            }
        }

        // printf("PARENT: Closing read end...\n");
        // fflush(stdout);
        close(command_input_pipe[PIPE_END_WRITE]);
        close(command_output_pipe[PIPE_END_READ]);

        // printf("Read from pipe, waiting...\n");

        fflush(NULL);
        int command_status;
        waitpid(cpid, &command_status, 0);
        if (command_status == -1) {
            printf("[TERM]: `waitpid` failure! pid=%d\n", (int)cpid);
            return;
        }

        // puts("Parent waited");
        // fflush(NULL);
    }
    else {
        // puts("Child");;
        close(command_input_pipe[PIPE_END_WRITE]);
        close(command_output_pipe[PIPE_END_READ]);

        // Redirect stdin to read end of command input pipe.
        syscall(SYS_repfd, command_input_pipe[PIPE_END_READ], STDIN_FILENO);
        close(command_input_pipe[PIPE_END_READ]);

        // Redirect stdout to write end of command output pipe.
        syscall(SYS_repfd, command_output_pipe[PIPE_END_WRITE], STDOUT_FILENO);
        close(command_output_pipe[PIPE_END_WRITE]);

        fflush(NULL);
        syscall(SYS_exec, filepath, args);
    }
}

/// @param filepath Passed to `exec` syscall
/// @param args
///   NULL-terminated array of pointers to NULL-terminated strings.
///   Passed to `exec` syscall
void run_background_program(const char* const filepath, const char** args) {
    if (fork() == 0) syscall(SYS_exec, filepath, args);
}

int main(int argc, const char** argv) {
    // Set stdout unbuffered so the user can see updates as they type.
    // NOTE: Probably not very efficient for the terminal's output to be unbuffered.
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // Get graphical window from opening a connection to the !GUI socket
    int sockFD = sys_socket(0, 0, 0);
    sockaddr addr;
    addr.type = LENSOR16;
    const char socket_path[] = "!GUI";
    memset(addr.data, 0, SOCK_ADDR_MAX_SIZE);
    memcpy(addr.data, &socket_path, sizeof(socket_path) - 1);
    int rc = sys_connect(sockFD, &addr, sizeof(sockaddr));
    if (rc) {
        close(sockFD);
        printf("[TERM]: Couldn't connect to GUI Server (address: %s)\n", socket_path);
        fflush(stdout);
        return rc;
    }
    unsigned char data[512];
    size_t bytes_read = 0;
    bytes_read += read(sockFD, data, 24);
    uint64_t* data_it = (uint64_t*)data;
    uint64_t shared_memory_id = data_it[2];
    uintptr_t* shared_data = (uintptr_t*)syscall(SYS_shared_memory_acquire, shared_memory_id);

    Framebuffer fb;
    fb.base_address = shared_data;
    fb.buffer_size = *shared_data++;
    fb.pixel_width = *shared_data++;
    fb.pixel_height = *shared_data++;
    fb.pixels_per_scanline = fb.pixel_width;
    // TODO: Pass format from server
    fb.format = FB_FORMAT_DEFAULT;
    g_framebuffer = fb;

    // clear screen
    const uint32_t black = mkpixel(fb.format, 22, 23, 24, 0xff);
    fill_color(fb, black);

    puts("\n\n[TERM]:\n<==!=!=<  WELCOME TO LensorOS  >=!=!==>\n");
    puts("  LensorOS  Copyright (C) 2022, Contributors To LensorOS.");

    const char* const fontpath = "/fs0/res/fonts/psf1/dfltfont.psf";
    FILE* fontfile = fopen(fontpath, "rb");
    if (!fontfile) {
        printf("[TERM]:Error: Could not open font at %s\n", fontpath);
        return 1;
    }
    printf("[TERM]: Successfully opened font at %s\n", fontpath);

    PSF1_FONT font;
    bytes_read = 0;
    bytes_read = fread(&font.header, 1, sizeof(PSF1_HEADER), fontfile);
    if (bytes_read != sizeof(PSF1_HEADER)) {
        printf("[TERM]:Error:Could not read PSF1 header from font file.\n");
        return 1;
    }
    if (font.header.magic[0] != PSF1_MAGIC0 || font.header.magic[1] != PSF1_MAGIC1) {
        printf("[TERM]:Error: Invalid font format (magic bytes not correct)\n");
        return 1;
    }

    size_t glyph_buffer_size = font.header.character_size * 256;
    // FIXME: This value checked against Mode may be wrong.
    if (font.header.mode == 1) {
        // 512 glyph mode
        glyph_buffer_size = font.header.character_size * 512;
    }

    // Read glyph buffer from font file after header
    font.glyph_buffer = malloc(glyph_buffer_size + sizeof(PSF1_HEADER));
    if (!font.glyph_buffer) {
        printf("[TERM]:Error: Failed to allocate memory for PSF1 font glyph buffer.\n");
        return 1;
    }

    fseek(fontfile, sizeof(PSF1_HEADER), SEEK_SET);
    bytes_read = fread(font.glyph_buffer, 1, glyph_buffer_size, fontfile);
    if (bytes_read != glyph_buffer_size) {
        printf("[TERM]:Error: Could not read PSF1 glyph buffer from font file.\n");
        return 1;
    }

    fclose(fontfile);

    g_font = font;
    printf("[TERM]: Successfully loaded PSF1 font from \"%s\"\n", fontpath);

    CharacterBuffer charbuf = charbuf_create(
        g_framebuffer.pixel_width / psf1_width(g_font),
        g_framebuffer.pixel_height / psf1_height(g_font));

    const char* sh_args[1] = {NULL};
    run_program_waitpid(sockFD, "/fs0/bin/xish", sh_args, &charbuf);

    printf("[TERM]: teardown\n");

    charbuf_delete(charbuf);
    psf1_delete(font);
    close(sockFD);

    return 0;
}
