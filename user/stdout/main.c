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

#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscalls.h>
#include <unistd.h>

#include "framebuffer.h"

/// Unsigned Integer Alias Declaration
typedef unsigned int uint;

/// Fixed-Width Unsigned Integer Alias Declarations
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uintptr_t usz;

/// Fixed-Width Signed Integer Alias Declarations
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef intptr_t ssz;

#ifndef MAX_COMMAND_LENGTH
# define MAX_COMMAND_LENGTH 4096
#endif

static char command[MAX_COMMAND_LENGTH];

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

/// @param filepath Passed to `exec` syscall
int run_program_waitpid(const char *const filepath) {
  // If there are pending writes, they will be executed on both the
  // parent and the child; by flushing any buffers we have, it ensures
  // the child won't write duplicate data on accident.
  pid_t cpid = syscall(SYS_fork);
  if (cpid) {
    //puts("Parent");
    //fflush(NULL);
    syscall(SYS_waitpid, cpid);
    //puts("Parent waited");
    //fflush(NULL);
  } else {
    //puts("Child");
    //fflush(NULL);
    syscall(SYS_exec, filepath);
  }
}

void print_command_line() {
  fputs("\033[2K", stdout); //> Erase entire line
  fputs("\033[1G", stdout); //> Move cursor to first column
  fputs("  $:", stdout);
  fputs(command, stdout);
  fflush(stdout);
}

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

    fprint_hexnibble(val, stdout);

    c = *(str++);
    if (c >= '0' && c <= '9') val = c - '0';
    else if (c >= 'A' && c <= 'F') val = 10 + c - 'A';
    else if (c >= 'a' && c <= 'f') val = 10 + c - 'a';
    else break;

    out <<= 4;
    out |= val;

    fprint_hexnibble(val, stdout);
  }
  putc('\n', stdout);

  return out;
}


#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04
typedef struct PSF1_HEADER {
  // Magic bytes to indicate PSF1 font type
  u8 magic[2];
  u8 mode;
  u8 character_size;
} PSF1_HEADER;
typedef struct PSF1_FONT {
  PSF1_HEADER header;
  void* glyph_buffer;
} PSF1_FONT;
void psf1_delete(const PSF1_FONT font) {
  free(font.glyph_buffer);
}
u8 psf1_width(const PSF1_FONT font) {
  return 8;
}
u8 psf1_height(const PSF1_FONT font) {
  return font.header.character_size;
}
/// bitmap size is as follows: (8, font.header->character_size)
/// @return address of beginning of bitmap pertaining to given character.
u8* psf1_char_bitmap(const PSF1_FONT font, const u8 c) {
  return (u8*)font.glyph_buffer + (c * psf1_height(font));
}

void draw_psf1_char(const Framebuffer fb, const PSF1_FONT font, size_t position_x, size_t position_y, const u8 c) {
  const u32 fg_color = mkpixel(fb.format, 0xff, 0xff, 0xff, 0xff);
  const u32 bg_color = mkpixel(fb.format, 0,0,0xff,0xff);

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


static const size_t prompt_start_x = 0;
static const size_t prompt_start_y = 0;
static const char *const prompt = "  $:";

void draw_prompt(Framebuffer fb, PSF1_FONT font) {
  size_t x = prompt_start_x;
  size_t y = prompt_start_y;

  for (const char *s = prompt; *s; ++s) {
    char c = *s;
    draw_psf1_char(fb, font, x, y, c);
    x += 8;
  }

  for (const char *s = command; *s; ++s) {
    char c = *s;
    draw_psf1_char(fb, font, x, y, c);
    if (c == '\n') {
      x = 0;
      y += font.header.character_size;
    }
    else if (c == '\r') x = 0;
    else x += 8;
  }
}


int main(int argc, const char **argv) {
  // TODO: If arguments are there, we should init framebuffer, draw to
  // it, etc. If it's not there, we should also be able to gracefully
  // handle that case.

  /*
  puts("Arguments:");
  for (int i = 0; i < argc; ++i) puts(argv[i]);
  fflush(NULL);
  */

  Framebuffer fb;
  fb.base_address        = (void *)hexstring_to_number(argv[1]);
  fb.buffer_size         = hexstring_to_number(argv[2]);
  fb.pixel_width         = hexstring_to_number(argv[3]);
  fb.pixel_height        = hexstring_to_number(argv[4]);
  fb.pixels_per_scanline = hexstring_to_number(argv[5]);
  // TODO: Pass format from kernel (which gets format passed from bootloader)
  fb.format = FB_FORMAT_DEFAULT;

  // clear screen
  const uint32_t black = mkpixel(fb.format, 0x00,0x00,0x00,0xff);
  fill_color(fb, black);

  puts("\n\n<===!= WELCOME TO LensorOS SHELL [WIP] =!=!==>\n");
  puts("  LensorOS  Copyright (C) 2022, Contributors To LensorOS.");

  const char *const fontpath = "/fs0/dfltfont.psf";
  FILE *fontfile = fopen(fontpath, "rb");
  if (!fontfile) {
    printf("Could not open font at %s\n", fontpath);
    return 1;
  }
  printf("Successfully loaded font at %s\n", fontpath);

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

  printf("Successfully loaded PSF1 font from \"%s\"\n", fontpath);

  // TODO: Very basic text editor implementation (GNU readline equivalent, basically).
  // |-- Moveable Cursor
  // |-- Insert/Delete byte at cursor
  // `-- GUI layout: place prompt always at bottom of screen, clear before redraw, etc.

  for (;;) {
    memset(command, 0, MAX_COMMAND_LENGTH);
    fputc('\n', stdout);
    fill_color(fb, black);
    print_command_line();
    draw_prompt(fb, font);

    // Get line from standard input.
    int c;
    int offset = 0;
    while ((c = getchar()) != '\n') {

      if (c == EOF) {
        // TODO: Wait/waste some time so we don't choke the system just
        // spinning.
        continue;
      }
      if (c == '\b') {
        if (offset > 0) {
          command[--offset] = '\0';
          // Echo command to standard out.
          print_command_line();
          // Draw a space over erased character (doesn't work over newline).
          draw_psf1_char(fb, font, prompt_start_x + ((strlen(prompt) + offset) * psf1_width(font)), prompt_start_y, ' ');
          draw_prompt(fb, font);
        }
        continue;
      }
      if (offset >= MAX_COMMAND_LENGTH) {
        puts("Reached max command length, discarding command.\n");
        offset = 0;
        command[offset] = '\0';
        print_command_line();
        draw_prompt(fb, font);
        continue;
      }
      command[offset++] = c;
      command[offset] = '\0';
      // Echo command to standard out.
      print_command_line();
      draw_prompt(fb, font);
    }
    command[offset] = '\0';

    // Finish printing command line.
    fputc('\n', stdout);

    if (strcmp(command, "blazeit") == 0) {
      run_program_waitpid("/fs0/blazeit");
      continue;
    }
    if (strcmp(command, "quit") == 0) {
      puts("Shell quitting, baiBAI!");
      fflush(NULL);
      break;
    }
    puts("Unrecognized command, sorry!\n"
         "  Try `blazeit` or `quit`");
    fflush(NULL);
    continue;
  }

  psf1_delete(font);

  return 0;
}
