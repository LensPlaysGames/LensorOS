#ifndef LENSOR_OS_IO_H
#define LENSOR_OS_IO_H

#include "integers.h"

void out8  (u16 port, u8 value);
u8   in8   (u16 port);
void out16 (u16 port, u16 value);
u16  in16  (u16 port);
void out32 (u16 port, u16 value);
u32  in32  (u16 port);

void io_wait();

#endif
