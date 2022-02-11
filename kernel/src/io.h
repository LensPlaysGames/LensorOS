#ifndef LENSOR_OS_IO_H
#define LENSOR_OS_IO_H

#include "integers.h"
#include "cstr.h"
#include "basic_renderer.h"

void outb(u16 port, u8 value);
uint8_t inb(u16 port);
void io_wait();

#endif
