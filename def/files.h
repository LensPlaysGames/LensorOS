#ifndef LENSOROS_DEFINES_FILES_H
#define LENSOROS_DEFINES_FILES_H

typedef enum FileType {
    Regular,
    Directory,
    Pipe,
    Socket,
} FileType;

typedef struct DirectoryEntry {
    FileType type;
    char name[252];
} DirectoryEntry;
static_assert(sizeof(DirectoryEntry) == 256, "DirectoryEntry has invalid size");
static_assert(offsetof(DirectoryEntry, name) == 4, "DirectoryEntry::name was padded");

#endif /* LENSOROS_DEFINES_FILES_H */
