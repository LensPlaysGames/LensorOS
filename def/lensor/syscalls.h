#ifndef LENSOROS_DEFINES_SYSCALLS_H
#define LENSOROS_DEFINES_SYSCALLS_H

typedef enum LensorOS_SyscallRead_Flags {
    LENSOROS_SYSCALL_READ_FLAG_NONE = 0,
    // If this bit is set, do *not* block to wait for data.
    LENSOROS_SYSCALL_READ_FLAG_NOBLOCK = 1 << 0,
} LensorOS_SyscallRead_Flags;

typedef enum LensorOS_SyscallWrite_Flags {
    LENSOROS_SYSCALL_WRITE_FLAG_NONE = 0,
    // If this bit is set, do *not* block to wait for room to write data.
    LENSOROS_SYSCALL_WRITE_FLAG_NOBLOCK = 1 << 0,
} LensorOS_SyscallWrite_Flags;

#endif /* LENSOROS_DEFINES_SYSCALLS_H */
