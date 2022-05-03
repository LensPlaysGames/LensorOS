#include <gdt.h>

GDT gGDT;
GDTDescriptor gGDTD;

void setup_gdt() {
    //                  BASE  LIMIT       ACCESS       FLAGS:LIMIT1
    gGDT.Null =      {  0,    0,          0x00,        0x00        };
    gGDT.Ring0Code = {  0,    0xffffffff, 0b10011010,  0b10110000  };
    gGDT.Ring0Data = {  0,    0xffffffff, 0b10010010,  0b10110000  };
    gGDT.Ring3Code = {  0,    0xffffffff, 0b11111010,  0b10110000  };
    gGDT.Ring3Data = {  0,    0xffffffff, 0b11110010,  0b10110000  };
    gGDT.TSS =       {{ 0,    0xffffffff, 0b10001001,  0b00100000 }};
}
