#include "io.h"

#include "basic_renderer.h"
#include "cstr.h"

void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
uint8_t inb(uint16_t port) {
    uint8_t retValue;
    asm volatile ("inb %1, %0" : "=a"(retValue) : "Nd"(port));
    return retValue;
}

void io_wait() {
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}
