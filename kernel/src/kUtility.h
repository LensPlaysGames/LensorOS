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

void kernel_init(BootInfo* info);
