#include <stdio.h>
#include <stddef.h>

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
        printf("\nbytes_read: %i  | ", bytes_read);
        fwrite(&c[0], 1, bytes_read, stdout);
    }

    printf("\nSo long, gay bowser\n");

    return 0;
}
