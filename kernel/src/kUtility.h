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

extern u64 KERNEL_START;
extern u64 KERNEL_END;
extern u64 TEXT_START;
extern u64 TEXT_END;
extern u64 DATA_START;
extern u64 DATA_END;
extern u64 READ_ONLY_DATA_START;
extern u64 READ_ONLY_DATA_END;
extern u64 BLOCK_STARTING_SYMBOLS_START;
extern u64 BLOCK_STARTING_SYMBOLS_END;

void kernel_init(BootInfo* info);
