#include <kstage1.h>
#include <memory/common.h>
#include <panic.h>

void kstage1(uintptr_t base, BootInfo* bInfo) {
    KERNEL_PHYSICAL = (uintptr_t)base;

    hang();
}
