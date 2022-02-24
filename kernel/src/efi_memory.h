#ifndef LENSOR_OS_EFI_MEMORY_OS
#define LENSOR_OS_EFI_MEMORY_OS

#include "integers.h"

struct EFI_MEMORY_DESCRIPTOR {
  u32 type;
  void* physicalAddress;
  void* virtualAddress;
  u64 numPages;
  u64 attributes;
};
extern const char* EFI_MEMORY_TYPE_STRINGS[];

void print_efi_memory_map(EFI_MEMORY_DESCRIPTOR* map, u64 mapSize, u64 mapDescSize);
void print_efi_memory_map_summed(EFI_MEMORY_DESCRIPTOR* map, u64 mapSize, u64 mapDescSize);

#endif
