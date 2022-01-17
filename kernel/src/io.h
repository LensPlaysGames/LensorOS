#ifndef LENSOR_OS_IO_H
#define LENSOR_OS_IO_H

#include <stdint.h>
#include "cstr.h"
#include "basic_renderer.h"

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void io_wait();

#endif
