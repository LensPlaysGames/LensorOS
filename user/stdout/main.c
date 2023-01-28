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
  fflush(NULL);
}

int main(int argc, const char **argv) {
  //puts("Arguments:");
  //for (int i = 0; i < argc; i++) puts(argv[i]);
  //fflush(NULL);

  puts("\n\n<== WELCOME TO LensorOS SHELL *WIP* ==>\n");

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
