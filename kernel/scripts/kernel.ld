OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)
SECTIONS
{
    KERNEL_PHYSICAL = 0x1000;
    KERNEL_VIRTUAL = 0xffffff8000000000;
    . = KERNEL_VIRTUAL + KERNEL_PHYSICAL;
    KERNEL_START = .;
    .text : AT(ADDR(.text) - KERNEL_VIRTUAL)
    {
        TEXT_START = .;
        *(.text*)
        *(.gnu.linkonce.t*)
        TEXT_END = .;
    }

    . = ALIGN(0x1000);
    .data : AT(ADDR(.data) - KERNEL_VIRTUAL)
    {
        DATA_START = .;
        *(.data*)
        *(.gnu.linkonce.d*)
        DATA_END = .;
    }

    . = ALIGN(0x1000);
    .rodata : AT(ADDR(.rodata) - KERNEL_VIRTUAL)
    {
        READ_ONLY_DATA_START = .;

        *(.rodata*)
        *(.gnu.linkonce.r*)
        READ_ONLY_DATA_END = .;
    }

    . = ALIGN(0x1000);
    .bss : AT(ADDR(.bss) - KERNEL_VIRTUAL)
    {
        BLOCK_STARTING_SYMBOLS_START = .;
        *(COMMON)
        *(.bss*)
        *(.gnu.linkonce.b*)
        BLOCK_STARTING_SYMBOLS_END = .;
    }
    KERNEL_END = .;

    /DISCARD/ :
    {
        *(.ctors)
        *(.dtors)
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
