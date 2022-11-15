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
#include <sys/types.h>
#include <sys/syscalls.h>

int main(int argc, const char **argv) {
  //const char *message = "Hello, friends :^)";
  //puts(message);
  for (int i = 0; i < argc; i++) puts(argv[i]);

  //for (;;) {
  //  // Get character from standard input.
  //  int c = getchar();
  //  // Skip error return value.
  //  if (c == EOF) {
  //    // TODO: Wait/waste some time so we don't choke the system just
  //    // spinning.
  //    continue;
  //  } else if (c == 'q') {
  //    puts("Quitting.");
  //    break;
  //  }
  //  // Echo character to standard out.
  //  putc((char)c, stdout);
  //  fflush(stdout);
  //}


  // If there are pending writes, they will be executed on both the
  // parent and the child; by flushing any buffers we have, it ensures
  // the child won't write duplicate data on accident.
  fflush(NULL);

  pid_t cpid = syscall(SYS_fork);
  //puts("fork returned");
  if (cpid) {
    puts("Parent");
  } else {
    puts("Child");
  }

  return 0;
}
