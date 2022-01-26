#ifndef LENSOR_OS_GDT_H
#define LENSOR_OS_GDT_H

#include "integers.h"

struct GDTDescriptor {
	u16 Size;
	u64 Offset;
} __attribute__((packed));

struct GDTEntry {
	u16 Limit0;
	u16 Base0;
	u8 Base1;
	u8 AccessByte;
	u8 Limit1_Flags;
	u8 Base2;
} __attribute__((packed));

// Global Descriptor Table
struct GDT {
	GDTEntry Null;
	GDTEntry KernelCode;
	GDTEntry KernelData;
	GDTEntry UserNull;
	GDTEntry UserCode;
	GDTEntry UserData;
}__attribute__((packed)) __attribute__((aligned(0x1000)));

extern GDT gGDT;

extern "C" void LoadGDT(GDTDescriptor* gdtDescriptor);

#endif
