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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

#include <stdbool.h>
#include <stdio.h>
#include <sys/syscalls.h>

#define PWD_MAX 4096
static char pwdbuf[PWD_MAX];

int main(void) {
  // TODO: Handle -L and -P arguments.
  pwdbuf[0] = '\0';
  if (!syscall(SYS_pwd, pwdbuf, PWD_MAX)) return 1;
  printf("%s\n", pwdbuf);
  return 0;
}
