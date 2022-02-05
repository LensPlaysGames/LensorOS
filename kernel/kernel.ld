OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    _KernelStart = .;
    .text : ALIGN(0x1000)
    {
    _TextStart = .;
        *(.text)
    _TextEnd = .;
    }

    .data : ALIGN(0x1000)
    {
    _DataStart = .;
        *(.data)
    _DataEnd = .;
    }


    .rodata : ALIGN(0x1000)
    {
    _ReadOnlyDataStart = .;
        *(.rodata)
    _ReadOnlyDataEnd = .;
    }

    .bss : ALIGN(0x1000)
    {
    _BlockStartingSymbolsStart = .;
        *(COMMON)
        *(.bss)
    _BlockStartingSymbolsEnd = .;
    }
    _KernelEnd = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
