#ifndef LENSOROS_DEFINES_FILES_H
#define LENSOROS_DEFINES_FILES_H

#include <stdint.h>

#ifndef __cplusplus
typedef uint64_t ProcessFileDescriptor;
typedef ProcessFileDescriptor ProcFD;
#else
typedef uint64_t FileDescriptor;

enum struct ProcessFileDescriptor : FileDescriptor { Invalid = static_cast<FileDescriptor>(-1) };
enum struct GlobalFileDescriptor : FileDescriptor { Invalid = static_cast<FileDescriptor>(-1) };

using ProcFD = ProcessFileDescriptor;
using SysFD = GlobalFileDescriptor;
#endif

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
