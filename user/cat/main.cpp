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

#include <stddef.h>
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Please provide exactly one filepath\n");
        return 1;
    }

    FILE* f = fopen(argv[1], "rb");
    if (not f) {
        printf("Could not open file at %s\n", argv[1]);
        return 1;
    }

    constexpr int bytes_to_read = 1;
    char c[bytes_to_read];
    size_t bytes_read;
    while ((bytes_read = fread(&c[0], 1, bytes_to_read, f)) > 0) {
        if (feof(f) or ferror(f) || bytes_read != bytes_to_read) break;
        fwrite(&c[0], 1, bytes_read, stdout);
    }

    return 0;
}
