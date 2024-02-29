#include <stdio.h>
#include <sys/syscalls.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Please provide exactly one filepath\n");
        return 1;
    }

    struct DirectoryEntry {
        uint32_t type;
        char name[248];
    };
    DirectoryEntry entries[8] = {0};

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
