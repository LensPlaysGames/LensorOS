.section .text
.align 2
.global _start           // Make the entry point visible to the linker
_start:
    // Disable all interrupts
    msr daifset, 3

    // Spin forever
kernel_loop:
    wfi
    b kernel_loop
