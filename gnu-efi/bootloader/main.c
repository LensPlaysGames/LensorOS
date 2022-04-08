#include <efi.h>
#include <efilib.h>
#include <elf.h>

// Data Types
#include <stddef.h>
#include <stdint.h>

#define KERNEL_VIRTUAL  0xffffffff80000000

EFI_HANDLE        gImageHandle;
EFI_SYSTEM_TABLE* gSystemTable;

int memcmp (const void* aptr, const void* bptr, size_t n) {
    const unsigned char* a = aptr;
    const unsigned char* b = bptr;
    for (size_t i = 0; i < n; i++) {
        if (a[i] < b[i]) { return -1; }
        else if (a[i] > b[i]) { return 1; }
    }
    return 0;
}

UINTN strcmp (CHAR8* a, CHAR8* b, UINTN length) {
    for (UINTN i = 0; i < length; ++i) {
        if (*a != *b)
            return 0;

        ++a;
        ++b;
    }
    return 1;
}

// LOAD FILE FROM EFI FILE SYSTEM
EFI_FILE* LoadFile(EFI_FILE* dir, CHAR16* path) {
    EFI_FILE* loadedFile;
    static EFI_LOADED_IMAGE_PROTOCOL* loadedImage;
    if (loadedImage == NULL) {
        gSystemTable
            ->BootServices
            ->HandleProtocol(gImageHandle,
                             &gEfiLoadedImageProtocolGuid,
                             (void**)&loadedImage);
    }
    static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystem;
    if (fileSystem == NULL) {
        gSystemTable
            ->BootServices
            ->HandleProtocol(loadedImage->DeviceHandle,
                             &gEfiSimpleFileSystemProtocolGuid,
                             (void**)&fileSystem);
    }
    if (dir == NULL) {
        fileSystem->OpenVolume(fileSystem, &dir);
    }

    EFI_STATUS status = dir->Open(dir, &loadedFile, path,
                                  EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if (status == EFI_SUCCESS) {
        return loadedFile;
    }
    return NULL;
}

typedef struct {
    void* BaseAddress;
    size_t BufferSize;
    unsigned int PixelWidth;
    unsigned int PixelHeight;
    unsigned int PixelsPerScanLine;
} Framebuffer;

#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04

typedef struct {
    // Magic bytes to indicate PSF1 font type
    unsigned char Magic[2];
    unsigned char Mode;
    unsigned char CharacterSize;
} PSF1_HEADER;

typedef struct {
    PSF1_HEADER* PSF1_Header;
    void* GlyphBuffer;
} PSF1_FONT;

PSF1_FONT* LoadPSF1Font(EFI_FILE* dir, CHAR16* path) {
    EFI_FILE* font = LoadFile(dir, path);
    if (font == NULL) { return NULL; }

    PSF1_HEADER* font_hdr;
    gSystemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(PSF1_HEADER), (void**)&font_hdr);
    UINTN size = sizeof(PSF1_HEADER);
    font->Read(font, &size, font_hdr);

    if (font_hdr->Magic[0] != PSF1_MAGIC0
        || font_hdr->Magic[1] != PSF1_MAGIC1)
    {
        Print(L"ERROR: Invalid font format");
        return NULL;
    }

    UINTN glyphBufferSize = font_hdr->CharacterSize * 256;
    if (font_hdr->Mode == 1) {
        // 512 glyph mode
        glyphBufferSize  = font_hdr->CharacterSize * 512;
    }

    // Read glyph buffer from font file after header
    void* glyphBuffer;
    // Eat header
    font->SetPosition(font, sizeof(PSF1_HEADER));
    gSystemTable->BootServices->AllocatePool(EfiLoaderData, glyphBufferSize, (void**)&glyphBuffer);
    font->Read(font, &glyphBufferSize, glyphBuffer);

    PSF1_FONT* final_font;
    gSystemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(PSF1_FONT), (void**)&final_font);
    final_font->PSF1_Header = font_hdr;
    final_font->GlyphBuffer = glyphBuffer;
    return final_font;
}

Framebuffer framebuffer;
Framebuffer* InitializeGOP() {
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    EFI_STATUS status;
 
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);
    if(EFI_ERROR(status)) {
        Print(L"ERROR: Unable to locate Graphics Output Protocol\n");
        return NULL;
    }
    Print(L"GOP located successfully\n");

    framebuffer.BaseAddress = (void*)gop->Mode->FrameBufferBase;
    framebuffer.BufferSize = gop->Mode->FrameBufferSize;
    framebuffer.PixelWidth = gop->Mode->Info->HorizontalResolution;
    framebuffer.PixelHeight = gop->Mode->Info->VerticalResolution;
    framebuffer.PixelsPerScanLine = gop->Mode->Info->PixelsPerScanLine;
    return &framebuffer;
}

typedef struct {
    Framebuffer* framebuffer;
    PSF1_FONT* font;
    EFI_MEMORY_DESCRIPTOR* map;
    UINTN mapSize;
    UINTN mapDescSize;
    void* RSDP;
} BootInfo;

EFI_STATUS efi_main (EFI_HANDLE IH, EFI_SYSTEM_TABLE* ST) {
    gImageHandle = IH;
    gSystemTable = ST;
    
    InitializeLib(gImageHandle, gSystemTable);

    Print(L"!==-- LensorOS BOOTLOADER --==!\n");

    // Attempt to find and load LensorOS directory in the root of the boot filesystem.
    EFI_FILE* bin = LoadFile(NULL, L"LensorOS");
    if (bin == NULL) {
        Print(L"ERROR: Could not load directory: \"/LensorOS/\"\n");
        return 1;
    }
    // Attempt to find and load kernel executable within the LensorOS directory.
    EFI_FILE* kernel = LoadFile(bin, L"kernel.elf");
    if (kernel == NULL) {
        Print(L"ERROR: Could not load kernel from /LensorOS/kernel.elf\n");
        return 1;
    }
    Print(L"Kernel has been found\n");

    // Read default font from root directory.
    PSF1_FONT* dflt_font = LoadPSF1Font(bin, L"dfltfont.psf");
    if (dflt_font == NULL) {
        Print(L"ERROR: Failed to load default font\n");
    }
    else {
        Print(L"Default font loaded successfully\n"
              L"  Mode:           %d\n"
              L"  Character Size: 8x%d\n",
              dflt_font->PSF1_Header->Mode,
              dflt_font->PSF1_Header->CharacterSize);
    }

    // Unload LensorOS directory.
    bin->Close(bin);

    // LOAD KERNEL ELF64 HEADER INTO MEMORY
    Elf64_Ehdr elf_header;
    UINTN elf64HeaderSize = sizeof(elf_header);
    kernel->Read(kernel, &elf64HeaderSize, &elf_header);

    /* Verify Elf64 Header from beginning of loaded kernel file:
     * |-- Has magic bytes at beginning of file.
     * |-- Is formatted with least significant byte at the lowest address.
     * |-- Is an executable.
     * `-- Was built for an x86_64 machine.
     * `-- Was built with this elf loader's version of ELF.    
     */
    if (memcmp(&elf_header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0
        || elf_header.e_ident[EI_CLASS] != ELFCLASS64
        || elf_header.e_ident[EI_DATA] != ELFDATA2LSB
        || elf_header.e_type != ET_EXEC
        || elf_header.e_machine != EM_X86_64
        || elf_header.e_version != EV_CURRENT)
    {
        Print(L"ERROR: Invalid kernel format\n");
        return 1;
    }
    Print(L"Kernel format verified successfully\n");

    // Find and load ELF program header from loaded and verified kernel's ELF header.
    Elf64_Phdr* program_hdrs;
    kernel->SetPosition(kernel, elf_header.e_phoff);
    UINTN programHeaderTableSize = elf_header.e_phnum * elf_header.e_phentsize;
    gSystemTable->BootServices->AllocatePool(EfiLoaderData, programHeaderTableSize, (void**)&program_hdrs);
    kernel->Read(kernel, &programHeaderTableSize, program_hdrs);

    // There is usually only ever one program header, but just in case I'll check all that exist.
    for (
        Elf64_Phdr* phdr = program_hdrs;
        (char*)phdr < (char*)program_hdrs + elf_header.e_phnum * elf_header.e_phentsize;
        phdr = (Elf64_Phdr*)((char*)phdr + elf_header.e_phentsize))
    {
        if (phdr->p_type == PT_LOAD) {
            // Allocate pages for program
            int pages = (phdr->p_memsz + 0x1000 - 1) / 0x1000;
            Elf64_Addr segment = phdr->p_paddr;
            gSystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);
            kernel->SetPosition(kernel, phdr->p_offset);
            UINTN size = phdr->p_filesz;
            kernel->Read(kernel, &size, (void*)segment);
            Print(L"LOADED: Kernel Program at 0x%x (%d bytes loaded, %d bytes allocated)\n", segment, size, phdr->p_memsz);
        }
    }
    Print(L"Kernel loaded successfully\n");

    // Initialize Unified Extensible Firmware Interface Graphics Output Protocol.
    // Let's call that the 'GOP' from now on.
    Framebuffer* gop_fb = InitializeGOP();
    Print(L"  Base: 0x%08x\n"
          L"  Size: 0x%x\n"
          L"  Pixel Width: %d\n"
          L"  Pixel Height: %d\n"
          L"  Pixels Per Scanline: %d\n",
          gop_fb->BaseAddress, 
          gop_fb->BufferSize, 
          gop_fb->PixelWidth, 
          gop_fb->PixelHeight, 
          gop_fb->PixelsPerScanLine);

    // Load EFI Memory map to pass to kernel.
    EFI_MEMORY_DESCRIPTOR* Map = NULL;
    UINTN MapSize, MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
    gSystemTable->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
    gSystemTable->BootServices->AllocatePool(EfiLoaderData, MapSize, (void**)&Map);
    gSystemTable->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
    Print(L"EFI memory map successfully parsed\n");

    // ACPI 2.0
    EFI_CONFIGURATION_TABLE* ConfigTable = gSystemTable->ConfigurationTable;
    void* rsdp = NULL;
    EFI_GUID ACPI2TableGuid = ACPI_20_TABLE_GUID;
    for (UINTN index = 0; index < gSystemTable->NumberOfTableEntries; index++) {
        if (CompareGuid(&ConfigTable[index].VendorGuid, &ACPI2TableGuid)) {
            if (strcmp((CHAR8*)"RSD PTR ", (CHAR8*)ConfigTable->VendorTable, 8)) {
                Print(L"Found Root System Descriptor Pointer (RSDP) Table 2.0");
                UINTN sum = 0;
                // Validate checksum
                // RSDP2.0 table = 36 bytes, trailing three are reserved (not included).
                for (UINT8 i = 0; i < 33; ++i)
                    sum += *((UINT8*)ConfigTable->VendorTable + i);

                UINT8 checksum = (UINT8)sum;
                if ((UINT8)checksum != 0) {
                    Print(L", but checksum is invalid (expecting zero)!\n");
                    Print(L"  Checksum: %d\n", checksum);
                }
                else {
                    Print(L" with valid checksum\n");
                    rsdp = (void*)ConfigTable->VendorTable;
                }
            }
        }
        ConfigTable++;
    }

    BootInfo info;
    info.framebuffer = gop_fb;
    info.font = dflt_font;
    info.map = Map;
    info.mapSize = MapSize;
    info.mapDescSize = DescriptorSize;
    info.RSDP = rsdp;

    // Exit boot services: free system resources dedicated to UEFI boot services,
    //   as well as prevent UEFI from shutting down automatically after 5 minutes.
    Print(L"Exiting boot services\n");
    gSystemTable->BootServices->ExitBootServices(gImageHandle, MapKey);

    // Define kernel entry point.
    void (*KernelStart)(BootInfo*) = ((__attribute__((sysv_abi)) void (*)(BootInfo*)) elf_header.e_entry - KERNEL_VIRTUAL);
    KernelStart(&info);

    // Once boot services have been exited, must never return!
    while (1)
        __asm__ ("hlt");

    return EFI_SUCCESS;
}
