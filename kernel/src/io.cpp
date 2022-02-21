#include "io.h"

#include "integers.h"

void out8(u16 port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void out16(u16 port, u16 value) {
    asm volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

void out32(u16 port, u32 value) {
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

u8 in8(u16 port) {
    u8 retValue;
    asm volatile ("inb %1, %0" : "=a"(retValue) : "Nd"(port));
    return retValue;
}

u16 in16(u16 port) {
    u16 retValue;
    asm volatile ("inw %1, %0" : "=a"(retValue) : "Nd"(port));
    return retValue;
}

u32 in32(u16 port) {
    u32 retValue;
    asm volatile ("inl %1, %0" : "=a"(retValue) : "Nd"(port));
    return retValue;
}

void io_wait() {
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}
