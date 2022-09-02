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
#ifndef LENSOR_OS_LINK_DEFINITIONS_H
#define LENSOR_OS_LINK_DEFINITIONS_H

#include <integers.h>

extern u64 KERNEL_PHYSICAL;
extern u64 KERNEL_VIRTUAL;
extern u64 KERNEL_START;
extern u64 KERNEL_END;
extern u64 TEXT_START;
extern u64 TEXT_END;
extern u64 DATA_START;
extern u64 DATA_END;
extern u64 READ_ONLY_DATA_START;
extern u64 READ_ONLY_DATA_END;
extern u64 BLOCK_STARTING_SYMBOLS_START;
extern u64 BLOCK_STARTING_SYMBOLS_END;

#define V2P(addr) ((u64)(addr) - (u64)&KERNEL_VIRTUAL)
#define P2V(addr) ((u64)(addr) + (u64)&KERNEL_VIRTUAL)

#endif /* LENSOR_OS_LINK_DEFINITIONS_H */
