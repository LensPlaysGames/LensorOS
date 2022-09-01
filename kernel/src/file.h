/* Copyright 2022, Contributors To LensorOS.
All rights reserved.

This file is part of LensorOS.

LensorOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LensorOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LensorOS. If not, see <https://www.gnu.org/licenses */
#ifndef LENSOR_OS_FILE_H
#define LENSOR_OS_FILE_H

#include <integers.h>
#include <pure_virtuals.h>

// A FileDescriptor is an index into the kernel's
// system-wide table of OpenFileDescriptions.
typedef u64 FileDescriptor;

class File {
public:
    File() {}

    FileDescriptor (*open)  () { nullptr };
    void           (*close) () { nullptr };
    void           (*read)  () { nullptr };
    void           (*write) () { nullptr };

    u64 flags()          { return Flags; }
    bool flag(u64 flag)  { return 1ull << flag; }

private:
    u64 Flags { 0 };
};

class Device : public File {
public:
    Device(u64 maj, u64 min)
        : Major(maj), Minor(min) {};
    Device(const File& f, u64 maj, u64 min)
        : File(f), Major(maj), Minor(min) {};

    u64 major() { return Major; }
    u64 minor() { return Minor; }

private:
    u64 Major { 0 };
    u64 Minor { 0 };
};

#endif /* LENSOR_OS_FILE_H */
