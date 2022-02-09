#include "gdt.h"

// 64-bit Mode ignores everything but access and flags.
__attribute__((aligned(0x1000)))
GDT gGDT = {
    //       ACCESS       FLAGS:LIMIT1
    {0,0,0,  0x00,        0x00,       0}, // NULL
    {0,0,0,  0b10011010,  0b10100000, 0}, // Ring0Code
    {0,0,0,  0b10010010,  0b10100000, 0}, // Ring0Data
    {0,0,0,  0x00,        0x00,       0}, // Ring3NULL
    {0,0,0,  0b11111010,  0b10100000, 0}, // Ring3Code
    {0,0,0,  0b11110010,  0b10100000, 0}, // Ring3Data
    {0,0,0,  0b10001001,  0b00100000, 0}, // TSS0
    {0,0,0,  0x00,        0x00,       0}  // TSS1
};
