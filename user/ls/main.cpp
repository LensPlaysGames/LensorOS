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

#include <stdio.h>
#include <sys/syscalls.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Please provide exactly one filepath\n");
        return 1;
    }

    DirectoryEntry entries[8] = {};

    int entry_count = syscall(SYS_directory_data, argv[1], &entries[0], 8);
    if (entry_count == -1) return 1;

    printf("%s:\n", argv[1]);
    for (int i = 0; i < entry_count; ++i) {
        const bool last_entry = i + 1 == entry_count;
        const bool directory = entries[i].type == 1;

        const char* indent = last_entry ? "`-- " : "|-- ";
        const char* dir_mark = directory ? "(D) " : "";

        printf("%s%s%s\n", indent, dir_mark, entries[i].name);
    }

    return 0;
}
