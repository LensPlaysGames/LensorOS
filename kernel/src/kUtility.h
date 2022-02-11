#include "integers.h"

struct Framebuffer;
struct PSF1_FONT;

struct EFI_MEMORY_DESCRIPTOR;
namespace ACPI {
    struct RSDP2;
}

struct BootInfo {
    Framebuffer* framebuffer;
    PSF1_FONT* font;
    EFI_MEMORY_DESCRIPTOR* map;
    u64 mapSize;
    u64 mapDescSize;
    ACPI::RSDP2* rsdp;
};

extern u64 _KernelStart;
extern u64 _KernelEnd;

extern u64 _TextStart;
extern u64 _TextEnd;
extern u64 _DataStart;
extern u64 _DataEnd;
extern u64 _ReadOnlyDataStart;
extern u64 _ReadOnlyDataEnd;
extern u64 _BlockStartingSymbolsStart;
extern u64 _BlockStartingSymbolsEnd;

void kernel_init(BootInfo* info);
