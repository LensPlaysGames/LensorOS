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
* along with LensorOS. If not, see <https://www.gnu.org/licenses
*/

#ifndef LENSOROS_VFS_FORWARD_H
#define LENSOROS_VFS_FORWARD_H

#include <file.h>

enum struct ProcessFileDescriptor : FileDescriptor { Invalid = static_cast<FileDescriptor>(-1) };
enum struct GlobalFileDescriptor : FileDescriptor { Invalid = static_cast<FileDescriptor>(-1) };

using ProcFD = ProcessFileDescriptor;
using SysFD = GlobalFileDescriptor;

#endif // LENSOROS_VFS_FORWARD_H
