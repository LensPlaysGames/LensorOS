#ifndef LENSOR_OS_IDT_H
#define LENSOR_OS_IDT_H

#include "../integers.h"

#define IDT_TA_InterruptGate       0x8e
#define IDT_TA_UserInterruptGate   0xee
#define IDT_TA_TrapGate            0x8f

struct IDTDescEntry {
  u16 offset0;
  u16 selector;
  u8  ist;
  u8  type_attr;
  u16 offset1;
  u32 offset2;
  u32 ignore;
  
  void SetOffset(u64 offset);
  u64 GetOffset();
};

struct IDTR {
  u16 Limit;
  u64 Offset;
} __attribute__((packed));

#endif
