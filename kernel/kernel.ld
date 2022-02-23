OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)
SECTIONS
{
    /* FIXME: Map kernel to something that isn't zero! */
    KERNEL_VIRTUAL = 0x0;
    . = KERNEL_VIRTUAL;
    KERNEL_START = .;
    .text : AT(ADDR(.text) - KERNEL_VIRTUAL)
    {
        TEXT_START = .;
        *(.text)
        TEXT_END = .;
        . = ALIGN(0x1000);
    }

    .data : AT(ADDR(.data) - KERNEL_VIRTUAL)
    {
        DATA_START = .;
        *(.data)
        DATA_END = .;
        . = ALIGN(0x1000);
    }

    .rodata : AT(ADDR(.rodata) - KERNEL_VIRTUAL)
    {
        READ_ONLY_DATA_START = .;
        *(.rodata)
        READ_ONLY_DATA_END = .;
        . = ALIGN(0x1000);
    }

    .bss : AT(ADDR(.bss) - KERNEL_VIRTUAL)
    {
        BLOCK_STARTING_SYMBOLS_START = .;
        *(COMMON)
        *(.bss)
        BLOCK_STARTING_SYMBOLS_END = .;
        . = ALIGN(0x1000);
    }
    KERNEL_END = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
