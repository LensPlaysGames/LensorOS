#ifndef LENSOR_OS_IDT_H
#define LENSOR_OS_IDT_H

#include <stdint.h>

#define IDT_TA_InterruptGate   0x8E
#define IDT_TA_TrapGate        0x8F

struct IDTDescEntry {
  uint16_t offset0;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset1;
  uint32_t offset2;
  uint32_t ignore;
  void SetOffset(uint64_t offset);
  uint64_t GetOffset();
};

struct IDTR {
  uint16_t Limit;
  uint64_t Offset;
} __attribute__((packed));

#endif
