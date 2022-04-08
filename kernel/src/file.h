#ifndef LENSOR_OS_FILE_H
#define LENSOR_OS_FILE_H

#include "integers.h"
#include "pure_virtuals.h"

// A FileDescriptor is an index into the kernel's
// system-wide table of OpenFileDescriptions.
typedef u64 FileDescriptor;

class File {
public:
    FileDescriptor(*open)();
    void(*close)();
    void(*read)();
    void(*write)();

    u64 flags() { return Flags; }

private:
    u64 Flags { 0 };
};

class Device : public File {
public:
    Device(u64 maj, u64 min)
        : Major(maj), Minor(min) {};
    Device(File f, u64 maj, u64 min)
        : File(f), Major(maj), Minor(min) {};

    u64 major() { return Major; }
    u64 minor() { return Minor; }

private:
    u64 Major { 0 };
    u64 Minor { 0 };
};

#endif /* LENSOR_OS_FILE_H */
