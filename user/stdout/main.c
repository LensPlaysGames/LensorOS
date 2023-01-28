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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscalls.h>

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

int main(int argc, const char **argv) {
  puts("Arguments:");
  for (int i = 0; i < argc; i++) puts(argv[i]);
  fflush(NULL);

  for (;;) {
    memset(command, 0, MAX_COMMAND_LENGTH);

    // Get line from standard input.
    int c;
    int offset = 0;
    while ((c = getchar()) != '\n') {
      if (c == EOF) {
        // TODO: Wait/waste some time so we don't choke the system just
        // spinning.
        continue;
      }
      if (c == '\b' && offset > 0) {
        command[--offset] = '\0';
        // Echo command to standard out.
        puts(command);
        fflush(NULL);
        continue;
      }
      if (offset >= MAX_COMMAND_LENGTH) {
        puts("Reached max command length, discarding command.");
        offset = 0;
        command[offset] = '\0';
        continue;
      }
      command[offset++] = c;
      // Echo command to standard out.
      puts(command);
      fflush(NULL);
    }
    command[offset] = '\0';

    if (strcmp(command, "blazeit") == 0) {
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
        syscall(SYS_exec, "/fs0/blazeit");
      }
      continue;
    } else if (strcmp(command, "quit") == 0) {
      puts("Shell quitting, baiBAI!\n");
      fflush(NULL);
      break;
    }
    puts("Unrecognized command, sorry!\n");
    fflush(NULL);
    continue;
  }

  return 0;
}
