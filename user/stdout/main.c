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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscalls.h>
#include <unistd.h>

#include <framebuffer.h>
#include <ints.h>
#include <psf.h>

// FIXME: This is a hack and should be removed
static Framebuffer g_framebuffer;
static PSF1_FONT g_font;

#define ESCAPE     0x01
#define BACKSPACE  0x0e
#define TAB        0x0f
#define ENTER      0x1c
#define LCONTROL   0x1d
#define LSHIFT     0x2a
#define RSHIFT     0x36
#define LALT       0x38
#define SPACE      0x39
#define CAPSLOCK   0x3a
#define NUMLOCK    0x45
#define SCROLLLOCK 0x46

/// Preceded by 'e0' byte.
#define ARROW_UP    0x48
#define ARROW_DOWN  0x50
#define ARROW_LEFT  0x4b
#define ARROW_RIGHT 0x4d

void fprint_hexnibble(unsigned char byte, FILE *f) {
  if (byte < 10) putc(byte + '0', f);
  else if (byte < 16) putc(byte - 10 + 'a', f);
  else putc('?', f);
}

unsigned int hex_value_digit(unsigned char value) {
  if (value < 10) return value + '0';
  else if (value < 16) return value - 10 + 'a';
  return -1;
}

void fprint_hexnumber(size_t number, FILE *f) {
  char leading = 1;
  for (size_t i = sizeof(size_t) - 1; i < sizeof(size_t); --i) {
    size_t value = (number >> (i * 8)) & 0xff;
    if (leading && !value) continue;
    else leading = 0;
    putc(hex_value_digit((number >> (4 + i * 8))  & 0x0f), f);
    putc(hex_value_digit((number >> (i * 8)) & 0x0f), f);
  }
  if (leading) {
    putc('0', f);
  }
}

unsigned int hex_digit_value(const char c) {
  if (c >= '0' && c <= '9') return c - '0';
  else if (c >= 'A' && c <= 'F') return 10 + c - 'A';
  else if (c >= 'a' && c <= 'f') return 10 + c - 'a';
  return -1;
}

size_t hexstring_to_number(const char *str) {
  size_t out = 0;
  // Skip `0x`
  if (*str == '0' && *(str + 1) == 'x') str += 2;

  char c;
  unsigned char val = 0;
  for (size_t i = sizeof(size_t) - 1; i < sizeof(size_t) ; --i) {
    c = *(str++);
    if (c >= '0' && c <= '9') val = c - '0';
    else if (c >= 'A' && c <= 'F') val = 10 + c - 'A';
    else if (c >= 'a' && c <= 'f') val = 10 + c - 'a';
    else break;

    out <<= 4;
    out |= val;

    //fprint_hexnibble(val, stdout);

    c = *(str++);
    if (c >= '0' && c <= '9') val = c - '0';
    else if (c >= 'A' && c <= 'F') val = 10 + c - 'A';
    else if (c >= 'a' && c <= 'f') val = 10 + c - 'a';
    else break;

    out <<= 4;
    out |= val;

    //fprint_hexnibble(val, stdout);
  }
  //putc('\n', stdout);

  return out;
}

void draw_psf1_char(const Framebuffer fb, const PSF1_FONT font, size_t position_x, size_t position_y, const u8 c) {
  const u32 fg_color = mkpixel(fb.format, 0xff, 0xff, 0xff, 0xff);
  const u32 bg_color = mkpixel(fb.format, 22,23,24,0xff);

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

static void draw_psf1_cr(const Framebuffer fb, const PSF1_FONT font, size_t *x) {
  *x = 0;
}
static void draw_psf1_lf(const Framebuffer fb, const PSF1_FONT font, size_t *y) {
  *y += font.header.character_size;
}
static void draw_psf1_crlf(const Framebuffer fb, const PSF1_FONT font, size_t *x, size_t *y) {
  // TODO: use bg color
  fill_rect(fb, mkpixel(fb.format, 22,23,24,255), *x, *y, fb.pixel_width, psf1_height(font));
  draw_psf1_cr(fb, font, x);
  draw_psf1_lf(fb, font, y);
}

void draw_psf1_string_view(Framebuffer fb, const PSF1_FONT font, size_t *x, size_t *y, const char *str, size_t length) {
  char c;
  while ((c = *str++) && length--) {
    if (c == '\n') draw_psf1_crlf(fb, font, x, y);
    else if (c == '\r') *x = 0;
    else if (c == '\b') *x -= psf1_width(font);
    else {
      draw_psf1_char(fb, font, *x, *y, c);
      *x += psf1_width(font);
    }
  }
}

void draw_psf1_string(Framebuffer fb, const PSF1_FONT font, size_t *x, size_t *y, const char *str) {
  char c;
  while ((c = *str++)) {
    if (c == '\n') draw_psf1_crlf(fb, font, x, y);
    else if (c == '\r') *x = 0;
    else if (c == '\b') *x -= psf1_width(font);
    else {
      draw_psf1_char(fb, font, *x, *y, c);
      *x += psf1_width(font);
    }
  }
}

void draw_psf1_int(Framebuffer fb, const PSF1_FONT font, size_t *x, size_t *y, int val) {
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
uint32_t *charbuf_at(const CharacterBuffer *charbuf, const size_t x, const size_t y) {
  return &charbuf->data[charbuf->width * y + x];
}
void charbuf_write(const CharacterBuffer *charbuf, const size_t x, const size_t y, const uint32_t c) {
  *charbuf_at(charbuf, x, y) = c;
}
void charbuf_putc(CharacterBuffer *charbuf, const uint32_t c) {
  if (c == '\b') {
    if (charbuf->cursor.x) {
      // Decrement cursor position
      charbuf->cursor.x -= 1;
      // Draw space over current cursor position
      charbuf_putc(charbuf, ' ');
      // Decrement cursor position (undo increment from writing space)
      charbuf->cursor.x -= 1;
    } else if (!charbuf->cursor.y) {
      // Backspace at very beginning of character buffer (replace first character with space).
      charbuf_putc(charbuf, ' ');
      charbuf->cursor.x = 0;
    } else {
      // TODO: At beginning of line, move cursor to end of last line.
      puts("\n[stdout]: TODO: backspace newline\n");
    }
  }
  else if (c == '\r') {
    charbuf->cursor.x = 0;
  } else if (c == '\n') {
    charbuf->cursor.y += 1;
    charbuf->cursor.x = 0;
    if (charbuf->cursor.y >= charbuf->height) {
      // FIXME: Hack for now to scroll entire character buffer.
      charbuf->cursor.y = 0;
      const uint32_t black = mkpixel(g_framebuffer.format, 22,23,24,0xff);
      fill_color(g_framebuffer, black);
    }
    // FIXME: We probably have to draw spaces to the rest of the line in the
    // character buffer.
  } else {
    // Write character into character buffer.
    charbuf_write(charbuf, charbuf->cursor.x, charbuf->cursor.y, c);

    // Draw graphical character into framebuffer at x, y position
    draw_psf1_char(g_framebuffer, g_font, charbuf->cursor.x * psf1_width(g_font), charbuf->cursor.y * psf1_height(g_font), c);

    // Update cursor position.
    charbuf->cursor.x += 1;
  }
}
void charbuf_puts(CharacterBuffer *charbuf, const uint32_t* s) {
  if (!s) return;
  while (*s)
    charbuf_putc(charbuf, *s);
}


/// @param filepath Passed to `exec` syscall
/// @param args
///   NULL-terminated array of pointers to NULL-terminated strings.
///   Passed to `exec` syscall
void run_program_waitpid(const char *const filepath, const char **args, CharacterBuffer* charbuf) {
  u64 fds[2] = {-1,-1};
  syscall(SYS_pipe, fds);

  pid_t cpid = fork();
  //printf("pid: %d\n", cpid);
  if (cpid) {
    //puts("Parent");
    //printf("PARENT: Closing write end...\n");
    //fflush(stdout);
    close(fds[1]);

    //printf("Reading from pipe!\n");
    //fflush(stdout);
    char c = 0;
    ssize_t bytes_read = 0;
    while ((bytes_read = read(fds[0], &c, 1)) && bytes_read != EOF) {
      // Draw output to stdout (probably DbgOutDriver, AKA UART).
      putc(c, stdout);
      charbuf_putc(charbuf, c);
    }

    //printf("PARENT: Closing read end...\n");
    //fflush(stdout);
    close(fds[0]);

    //printf("Read from pipe, waiting...\n");

    // TODO: waitpid needs to reserve some uncommon error code for
    // itself so that it is clear what is a failure from waitpid or just a
    // failing status. Maybe have some other way to check? Or wrap this in
    // libc that sets errno (that always goes well).
    fflush(NULL);
    int command_status = (int)syscall(SYS_waitpid, cpid);
    if (command_status == -1) {
      printf("`waitpid` failure! pid=%d\n", (int)cpid);
      return;
    }

    //puts("Parent waited");
    //fflush(NULL);

  } else {
    //puts("Child");;
    close(fds[0]);

    // Redirect stdout to write end of pipe.
    syscall(SYS_repfd, fds[1], STDOUT_FILENO);
    close(fds[1]);

    fflush(NULL);
    syscall(SYS_exec, filepath, args);
  }
}

/// @param filepath Passed to `exec` syscall
/// @param args
///   NULL-terminated array of pointers to NULL-terminated strings.
///   Passed to `exec` syscall
void run_background_program(const char *const filepath, const char **args) {
  if (fork() == 0) syscall(SYS_exec, filepath, args);
}

int main(int argc, const char **argv) {
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
  fb.buffer_size         = hexstring_to_number(argv[2]);
  fb.pixel_width         = hexstring_to_number(argv[3]);
  fb.pixel_height        = hexstring_to_number(argv[4]);
  fb.pixels_per_scanline = hexstring_to_number(argv[5]);
  // TODO: Pass format from kernel (which gets format passed from bootloader)
  fb.format = FB_FORMAT_DEFAULT;
  g_framebuffer = fb;

  // clear screen
  const uint32_t black = mkpixel(fb.format, 22,23,24,0xff);
  fill_color(fb, black);

  puts("\n\n<===!= WELCOME TO LensorOS SHELL [WIP] =!=!==>\n");
  puts("  LensorOS  Copyright (C) 2022, Contributors To LensorOS.");

  const char *const fontpath = "/fs0/res/fonts/psf1/dfltfont.psf";
  FILE *fontfile = fopen(fontpath, "rb");
  if (!fontfile) {
    printf("Could not open font at %s\n", fontpath);
    return 1;
  }
  printf("Successfully opened font at %s\n", fontpath);

  PSF1_FONT font;
  size_t bytes_read = 0;
  bytes_read = fread(&font.header, 1, sizeof(PSF1_HEADER), fontfile);
  if (bytes_read != sizeof(PSF1_HEADER)) {
    printf("Could not read PSF1 header from font file.\n");
    return 1;
  }
  if (font.header.magic[0] != PSF1_MAGIC0 || font.header.magic[1] != PSF1_MAGIC1) {
    printf("Invalid font format (magic bytes not correct)\n");
    return 1;
  }

  size_t glyph_buffer_size = font.header.character_size * 256;
  // FIXME: This value checked against Mode may be wrong.
  if (font.header.mode == 1) {
    // 512 glyph mode
    glyph_buffer_size  = font.header.character_size * 512;
  }

  // Read glyph buffer from font file after header
  font.glyph_buffer = malloc(glyph_buffer_size + sizeof(PSF1_HEADER));
  if (!font.glyph_buffer) {
    printf("Failed to allocate memory for PSF1 font glyph buffer.\n");
    return 1;
  }

  fseek(fontfile, sizeof(PSF1_HEADER), SEEK_SET);
  bytes_read = fread(font.glyph_buffer, 1, glyph_buffer_size, fontfile);
  if (bytes_read != glyph_buffer_size) {
    printf("Could not read PSF1 glyph buffer from font file.\n");
    return 1;
  }

  fclose(fontfile);

  g_font = font;
  printf("Successfully loaded PSF1 font from \"%s\"\n", fontpath);

  CharacterBuffer charbuf = charbuf_create(psf1_width(g_font), psf1_height(g_font));

  const char* sh_args[1] = {NULL};
  run_program_waitpid("/fs0/bin/xish", sh_args, &charbuf);

  // TODO: Uhhh, init shell exited. Should we try to reboot into another shell?
  // Hang for now
  for (;;)
    ;

  charbuf_delete(charbuf);
  psf1_delete(font);

  return 0;
}
