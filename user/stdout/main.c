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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscalls.h>
#include <unistd.h>

#include "framebuffer.h"

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

// FIXME: I don't think this works :(
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

int main(int argc, const char **argv) {
  puts("Arguments:");
  for (int i = 0; i < argc; ++i) puts(argv[i]);
  fflush(NULL);

  Framebuffer fb;
  fb.base_address        = (void *)hexstring_to_number(argv[0]);
  fb.buffer_size         = hexstring_to_number(argv[1]);
  fb.pixel_width         = hexstring_to_number(argv[2]);
  fb.pixel_height        = hexstring_to_number(argv[3]);
  fb.pixels_per_scanline = hexstring_to_number(argv[4]);
  // TODO: Pass format from kernel (which gets format passed from bootloader)
  fb.format = FB_FORMAT_DEFAULT;

  puts("\n\n<===!= WELCOME TO LensorOS SHELL [WIP] =!=!==>\n");
  puts("LensorOS  Copyright (C) 2022, Contributors To LensorOS.");

  // clear screen
  uint32_t black = mkpixel(fb.format, 0x00,0x00,0x00,0xff);
  fill_color(fb, black);

  for (;;) {
    memset(command, 0, MAX_COMMAND_LENGTH);
    fputc('\n', stdout);
    print_command_line();

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
        }
        continue;
      }
      if (offset >= MAX_COMMAND_LENGTH) {
        puts("Reached max command length, discarding command.\n");
        offset = 0;
        command[offset] = '\0';
        print_command_line();
        continue;
      }
      command[offset++] = c;
      command[offset] = '\0';
      // Echo command to standard out.
      print_command_line();
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

  return 0;
}
