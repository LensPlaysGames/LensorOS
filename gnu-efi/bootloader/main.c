#include <efi.h>
#include <efilib.h>
#include <elf.h>

// Data Types
#include <stddef.h>
#include <stdint.h>

#define ENTRY_BASE_TYPE(NAME) void (*NAME)(uintptr_t base, BootInfo*)

#if defined(aarch64)
#define HALT(...) __asm__ volatile("wfi")
#define ENTRY_TYPE(NAME) ENTRY_BASE_TYPE(NAME)
#define ELF_MACHINE EM_AARCH64
// TODO: Don't hard-code. Get from linker script or something.
#define KERNEL_VIRTUAL 0xffff000000000000
#elif defined(x86_64)
#define HALT(...) __asm__ volatile("hlt")
#define ENTRY_TYPE(NAME) __attribute__((sysv_abi)) ENTRY_BASE_TYPE(NAME)
#define ELF_MACHINE EM_X86_64
// TODO: Don't hard-code. Get from linker script or something.
#define KERNEL_VIRTUAL 0xffffffff80000000
#endif

#define V2P(ADDRESS) ((ADDRESS) - (KERNEL_VIRTUAL - kernel_info.base))

#define DEBUG_EFI_GOP
#define DEBUG_EFI_CONFIG_TABLE
// #define DEBUG_EFI_MEMORY_MAP

#ifdef DEBUG_EFI_MEMORY_MAP
const CHAR16* GetMemoryTypeString(UINT32 type) {
    static const CHAR16* memory_types[] = {
        L"EfiReservedMemoryType",
        L"EfiLoaderCode",
        L"EfiLoaderData",
        L"EfiBootServicesCode",
        L"EfiBootServicesData",
        L"EfiRuntimeServicesCode",
        L"EfiRuntimeServicesData",
        L"EfiConventionalMemory",
        L"EfiUnusableMemory",
        L"EfiACPIReclaimMemory",
        L"EfiACPIMemoryNVS",
        L"EfiMemoryMappedIO",
        L"EfiMemoryMappedIOPortSpace",
        L"EfiPalCode",
        L"EfiPersistentMemory"};

    // Make sure the type is within standard specification limits
    if (type < (sizeof(memory_types) / sizeof(memory_types[0]))) {
        return memory_types[type];
    }

    // Handle OEM/OS-specific or invalid types safely
    if (type >= 0x80000000) {
        return L"OEM_ReservedMemoryType";
    }

    return L"EfiMaxMemoryType / Unknown";
}
#endif

#define GUIDFMT L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define GUIDFMT_UNPACK(GUIDPTR)                                        \
    (GUIDPTR)->Data1,                                                  \
        (GUIDPTR)->Data2,                                              \
        (GUIDPTR)->Data3,                                              \
        (GUIDPTR)->Data4[0], (GUIDPTR)->Data4[1],                      \
        (GUIDPTR)->Data4[2], (GUIDPTR)->Data4[3], (GUIDPTR)->Data4[4], \
        (GUIDPTR)->Data4[5], (GUIDPTR)->Data4[6], (GUIDPTR)->Data4[7]

// https://uefi.org/specs/UEFI/2.10/04_EFI_System_Table.html#industry-standard-configuration-tables
#define EFI_ACPI_TABLE_GUID \
    {0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81}}

// https://uefi.org/specs/UEFI/2.10/04_EFI_System_Table.html#devicetree-tables
#define EFI_DEVICE_TREE_GUID \
    {0xb1b621d5, 0xf19c, 0x41a5, {0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0}}

EFI_HANDLE gImageHandle;
EFI_SYSTEM_TABLE* gSystemTable;
EFI_BOOT_SERVICES* gBootServices;

int memcmp(const void* aptr, const void* bptr, size_t n) {
    const unsigned char* a = aptr;
    const unsigned char* b = bptr;
    for (size_t i = 0; i < n; i++) {
        if (a[i] < b[i]) {
            return -1;
        }
        else if (a[i] > b[i]) {
            return 1;
        }
    }
    return 0;
}

UINTN strcmp(CHAR8* a, CHAR8* b, UINTN length) {
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
        gBootServices->HandleProtocol(gImageHandle,
                                      &gEfiLoadedImageProtocolGuid,
                                      (void**)&loadedImage);
    }
    static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystem;
    if (fileSystem == NULL) {
        gBootServices->HandleProtocol(loadedImage->DeviceHandle,
                                      &gEfiSimpleFileSystemProtocolGuid,
                                      (void**)&fileSystem);
    }
    if (dir == NULL) {
        fileSystem->OpenVolume(fileSystem, &dir);
    }

    EFI_STATUS status = dir->Open(dir, &loadedFile, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
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
    if (font == NULL) {
        return NULL;
    }

    PSF1_HEADER* font_hdr;
    gBootServices->AllocatePool(EfiLoaderData, sizeof(PSF1_HEADER), (VOID**)&font_hdr);
    if (font_hdr == 0) {
        Print(L"ERROR: Failed to allocate pool for PSF1 font header.\n");
        return NULL;
    }

    UINTN size = sizeof(PSF1_HEADER);
    font->Read(font, &size, font_hdr);

    if (font_hdr->Magic[0] != PSF1_MAGIC0
        || font_hdr->Magic[1] != PSF1_MAGIC1) {
        Print(L"ERROR: Invalid font format");
        return NULL;
    }

    UINTN glyphBufferSize = font_hdr->CharacterSize * 256;
    // FIXME: This value checked against Mode may be wrong.
    if (font_hdr->Mode == 1) {
        // 512 glyph mode
        glyphBufferSize = font_hdr->CharacterSize * 512;
    }

    // Read glyph buffer from font file after header
    VOID* glyphBuffer = NULL;
    gBootServices->AllocatePool(EfiLoaderData, glyphBufferSize, &glyphBuffer);
    if (glyphBuffer == NULL) {
        Print(L"ERROR: Failed to allocate pool for PSF1 font glyph buffer.\n");
        return NULL;
    }
    font->SetPosition(font, sizeof(PSF1_HEADER));
    font->Read(font, &glyphBufferSize, glyphBuffer);

    PSF1_FONT* final_font = NULL;
    gBootServices->AllocatePool(
        EfiLoaderData,
        sizeof(PSF1_FONT),
        (VOID**)&final_font);
    if (final_font == NULL) {
        Print(L"ERROR: Failed to allocate pool for final PSF1 font.\n");
        return NULL;
    }
    final_font->PSF1_Header = font_hdr;
    final_font->GlyphBuffer = glyphBuffer;
    return final_font;
}

Framebuffer framebuffer;
Framebuffer* InitializeGOP() {
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    EFI_STATUS status;

    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);
    if (EFI_ERROR(status)) {
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

typedef struct EfiMemoryMap {
    EFI_MEMORY_DESCRIPTOR* Entry;
    UINTN MapSize;
    UINTN DescriptorSize;
    UINTN Key;
} EfiMemoryMap;

EfiMemoryMap GetMemoryMap() {
    EfiMemoryMap Map;
    Map.Entry = NULL;
    Map.MapSize = 0;
    Map.DescriptorSize = 0;

    UINT32 DescriptorVersion;
    EFI_STATUS Status;

    // Get the initial size required for the memory map
    gBootServices->GetMemoryMap(
        &Map.MapSize,
        NULL,
        &Map.Key,
        &Map.DescriptorSize,
        &DescriptorVersion);

    Status = EFI_BUFFER_TOO_SMALL;
    while (Status == EFI_BUFFER_TOO_SMALL) {
        // Add extra padding room because AllocatePool itself can grow the map.
        Map.MapSize += 2 * Map.DescriptorSize;

        // Allocate the memory for the map itself.
        Status = gBootServices->AllocatePool(
            EfiLoaderData,
            Map.MapSize,
            (void**)&Map.Entry);

        if (EFI_ERROR(Status)) {
            Print(L"Failed to allocate pool for memory map\n");
            return Map;
        }

        // Try to fetch the map into the allocated buffer
        Status = gBootServices->GetMemoryMap(
            &Map.MapSize,
            Map.Entry,
            &Map.Key,
            &Map.DescriptorSize,
            &DescriptorVersion);

        // If it failed because the map grew again, free the buffer and try again
        if (Status == EFI_BUFFER_TOO_SMALL) {
            gBootServices->FreePool(Map.Entry);
            Map.Entry = NULL;
        }
    }

    if (EFI_ERROR(Status)) {
        Print(L"Failed to retrieve EFI memory map: %r\n", Status);
        if (Map.Entry) {
            gBootServices->FreePool(Map.Entry);
            Map.Entry = NULL;
        }
    }
    else
        Print(L"EFI memory map successfully parsed\n");

    return Map;
}

typedef struct KernelInfo {
    UINTN base;
    UINTN entry;
    UINTN size;  // Added to help track the footprint
} KernelInfo;

EFI_STATUS LoadKernel(EFI_FILE* dir, KernelInfo* kernel_info) {
    if (dir == nullptr || kernel_info == nullptr)
        return EFI_INVALID_PARAMETER;

    EFI_FILE* kernel = LoadFile(dir, L"kernel.elf");
    if (kernel == nullptr) {
        Print(L"ERROR: Could not load kernel from /LensorOS/kernel.elf\n");
        return EFI_NOT_FOUND;
    }

    Elf64_Ehdr elf_header;
    UINTN elf64HeaderSize = sizeof(elf_header);
    EFI_STATUS rc = kernel->Read(kernel, &elf64HeaderSize, &elf_header);
    if (rc != EFI_SUCCESS) {
        Print(L"ERROR: Could not read ELF header from kernel\n");
        return rc;
    }

    if (memcmp(&elf_header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0
        || elf_header.e_ident[EI_CLASS] != ELFCLASS64
        || elf_header.e_ident[EI_DATA] != ELFDATA2LSB
        || elf_header.e_type != ET_EXEC
        || elf_header.e_machine != ELF_MACHINE
        || elf_header.e_version != EV_CURRENT) {
        Print(L"ERROR: Invalid kernel format\n");
        return EFI_NOT_FOUND;
    }

    kernel_info->entry = elf_header.e_entry;

    // Load ELF program header table.
    Elf64_Phdr* program_hdrs;
    rc = kernel->SetPosition(kernel, elf_header.e_phoff);
    if (rc != EFI_SUCCESS) return rc;

    UINTN programHeaderTableSize = elf_header.e_phnum * elf_header.e_phentsize;
    gBootServices->AllocatePool(EfiLoaderData, programHeaderTableSize, (void**)&program_hdrs);
    if (program_hdrs == nullptr) return EFI_NOT_FOUND;

    rc = kernel->Read(kernel, &programHeaderTableSize, program_hdrs);
    if (rc != EFI_SUCCESS) return rc;

    // Calculate the entire span of the kernel
    Elf64_Addr min_vaddr = (Elf64_Addr)-1;
    Elf64_Addr max_vaddr = 0;
    BOOLEAN has_loadable_segments = FALSE;

    for (UINTN i = 0; i < elf_header.e_phnum; i++) {
        Elf64_Phdr* phdr = (Elf64_Phdr*)((char*)program_hdrs + (i * elf_header.e_phentsize));
        if (phdr->p_type != PT_LOAD) continue;

        has_loadable_segments = TRUE;
        if (phdr->p_vaddr < min_vaddr) {
            min_vaddr = phdr->p_vaddr;
        }
        if (phdr->p_vaddr + phdr->p_memsz > max_vaddr) {
            max_vaddr = phdr->p_vaddr + phdr->p_memsz;
        }
    }

    if (!has_loadable_segments) {
        Print(L"ERROR: No loadable segments found in ELF\n");
        return EFI_NOT_FOUND;
    }

    UINTN total_kernel_size = max_vaddr - min_vaddr;
    UINTN total_pages = (total_kernel_size + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE;

    // Allocate one single contiguous block for kernel image
    Elf64_Addr contiguous_physical_base;
    rc = gBootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, total_pages, &contiguous_physical_base);
    if (rc != EFI_SUCCESS) {
        Print(L"ERROR: Failed to allocate %d contiguous pages for kernel block\n", total_pages);
        return rc;
    }

    // Zero out the entire allocated region (wipes .bss automatically)
    gBootServices->SetMem((VOID*)contiguous_physical_base, total_pages * EFI_PAGE_SIZE, 0);

    kernel_info->base = contiguous_physical_base;
    kernel_info->size = total_kernel_size;

    // Copy segments relative to the new contiguous base
    for (UINTN i = 0; i < elf_header.e_phnum; i++) {
        Elf64_Phdr* phdr = (Elf64_Phdr*)((char*)program_hdrs + (i * elf_header.e_phentsize));
        if (phdr->p_type != PT_LOAD) continue;

        // Calculate destination based on the segment's offset from the lowest virtual address
        UINTN relative_offset = phdr->p_vaddr - min_vaddr;
        VOID* dest_address = (VOID*)(contiguous_physical_base + relative_offset);

        if (phdr->p_filesz > 0) {
            rc = kernel->SetPosition(kernel, phdr->p_offset);
            if (rc != EFI_SUCCESS) return rc;

            UINTN file_size = phdr->p_filesz;
            rc = kernel->Read(kernel, &file_size, dest_address);
            if (rc != EFI_SUCCESS || file_size != phdr->p_filesz) {
                Print(L"ERROR: Failed to read segment data\n");
                return rc;
            }
        }

        Print(L"SEGMENT LOADED: Virtual 0x%016lx mapped to Physical 0x%016lx (Size: %d bytes)\n",
              phdr->p_vaddr,
              (UINTN)dest_address,
              phdr->p_memsz);
    }

    gBootServices->FreePool(program_hdrs);
    Print(L"Kernel contiguously loaded successfully at physical 0x%016lx\n", contiguous_physical_base);

    return EFI_SUCCESS;
}

typedef struct {
    /// GOP Pointer (UEFI Graphics Output Protocol)
    /// Linear Pixel Framebuffer
    Framebuffer* framebuffer;

    void* devices; /**< Firmware device description. **/
                   /**< RSDP, Device Tree Blob, etc. **/

    uintptr_t base;  /**< Base address of loaded kernel image. **/
    PSF1_FONT* font; /**< Base address of loaded PSF font. **/

    EFI_MEMORY_DESCRIPTOR* map;
    UINTN mapSize;
    UINTN mapDescSize;

} BootInfo;

EFI_STATUS efi_main(EFI_HANDLE IH, EFI_SYSTEM_TABLE* ST) {
    gImageHandle = IH;
    gSystemTable = ST;
    gBootServices = gSystemTable->BootServices;

    InitializeLib(gImageHandle, gSystemTable);

    Print(L"!==-- LensorOS BOOTLOADER --==!\n");

    BootInfo info;
    info.framebuffer = nullptr;
    info.font = nullptr;
    info.map = nullptr;
    info.devices = nullptr;

    /** Fetch Linear Framebuffer
     **
     ** This is a direct, memory-accessible pixel buffer we can use to draw
     ** to "the screen", whatever the firmware of the machine has decided that
     ** is.
     **/
    info.framebuffer = InitializeGOP();
#ifdef DEBUG_EFI_GOP
    Print(
        L"  Base: 0x%08x\n"
        L"  Size: 0x%x\n"
        L"  Pixel Width: %d\n"
        L"  Pixel Height: %d\n"
        L"  Pixels Per Scanline: %d\n",
        info.framebuffer->BaseAddress,
        info.framebuffer->BufferSize,
        info.framebuffer->PixelWidth,
        info.framebuffer->PixelHeight,
        info.framebuffer->PixelsPerScanLine);
#endif

    /** UEFI Configuration Table
     **
     ** Within the UEFI System Table, the Configuration Table provides a
     ** mapping of GUIDs to vendor-defined pointers.
     **
     ** ACPI provides RSDP 2.0 base pointer (or, _should_).
     ** Device Tree provides FDT base pointer (or, _should_).
     **/

#ifdef DEBUG_EFI_CONFIG_TABLE
#define Print(...) Print(__VA_ARGS__)
#else
#define Print(...)
#endif

    Print(L"Configuration Table:\n");

    EFI_GUID guid_acpi2 = EFI_ACPI_TABLE_GUID;
    EFI_GUID guid_fdt = EFI_DEVICE_TREE_GUID;
    for (UINTN i = 0; i < gSystemTable->NumberOfTableEntries; ++i) {
        EFI_CONFIGURATION_TABLE* config_entry = &gSystemTable->ConfigurationTable[i];

        Print(
            L"  " GUIDFMT L": 0x%016llx\n",
            GUIDFMT_UNPACK(&config_entry->VendorGuid),
            config_entry->VendorTable);

        if (CompareGuid(&guid_acpi2, &config_entry->VendorGuid) == 0) {
            Print(L"  -- ACPI 2.0\n");
            info.devices = config_entry->VendorTable;
        }
        else if (CompareGuid(&guid_fdt, &config_entry->VendorGuid) == 0) {
            Print(L"  -- Device Tree\n");
            info.devices = config_entry->VendorTable;
        }
    }
#undef Print

    /** Load Kernel Image and Associated Resources
     **
     **/
    EFI_FILE* dir = LoadFile(nullptr, L"LensorOS");
    if (!dir) {
        Print(L"ERROR: Could not load directory: \"/LensorOS/\"\n");
        return EFI_LOAD_ERROR;
    }

    KernelInfo kernel_info = {};
    EFI_STATUS kernel_loaded = LoadKernel(dir, &kernel_info);
    info.base = kernel_info.base;
    info.font = LoadPSF1Font(dir, L"dfltfont.psf");

    dir->Close(dir);

    if (kernel_loaded != EFI_SUCCESS) {
        Print(L"ERROR: Could not load kernel from /LensorOS/kernel.elf\n");
        return EFI_LOAD_ERROR;
    }
    Print(L"kbase:0x%016llx kentry:0x%016llx\n", kernel_info.base, kernel_info.entry);

    if (!info.font) {
        Print(L"ERROR: Could not load font from /LensorOS/dfltfont.psf\n");
        return EFI_LOAD_ERROR;
    }

    /** Fetch EFI Memory Map
     **
     ** This is a firmware description of the valid and invalid memory regions
     ** present on the current machine; the kernel will use this to initialize
     ** it's memory tracking structures.
     **/
    EfiMemoryMap Map = GetMemoryMap();
    info.map = Map.Entry;
    info.mapSize = Map.MapSize;
    info.mapDescSize = Map.DescriptorSize;

#ifdef DEBUG_EFI_MEMORY_MAP
    Print(L"Memory Map:\n");
    for (
        UINT8* map_ptr = (UINT8*)info.map;
        map_ptr < ((UINT8*)info.map) + info.mapSize;
        map_ptr += info.mapDescSize) {
        EFI_MEMORY_DESCRIPTOR* map_entry = (EFI_MEMORY_DESCRIPTOR*)map_ptr;
        Print(
            L"  %s: %llu PAGES VIRT:0x%016llx PHYS:0x%016llx ATTR:0x%llx\n",
            GetMemoryTypeString(map_entry->Type),
            map_entry->NumberOfPages,
            map_entry->VirtualStart,
            map_entry->PhysicalStart,
            map_entry->Attribute);
    }
#endif

    Print(L"Kernel entry point: 0x%x\n", kernel_info.entry);
    Print(L"Calculated kernel entry point: 0x%x\n", V2P(kernel_info.entry));

    UINTN physical_kernel_entry = V2P(kernel_info.entry);
    UINTN kernel_end = kernel_info.base + kernel_info.size;
    if (physical_kernel_entry < kernel_info.base || physical_kernel_entry >= kernel_end) {
        Print(
            L"!! Calculated kernel entry, 0x%llx, is not within kernel region 0x%llx -- 0x%llx\n",
            physical_kernel_entry,
            kernel_info.base,
            kernel_info.base + kernel_info.size);
        return EFI_LOAD_ERROR;
    }

    // Exit boot services: free system resources dedicated to UEFI boot services,
    //   as well as prevent UEFI from shutting down automatically after 5 minutes.
    Print(L"Exiting boot services\n");
    gBootServices->ExitBootServices(gImageHandle, Map.Key);

    // Define kernel entry point.
    ENTRY_TYPE(KernelStart) = ((ENTRY_TYPE())physical_kernel_entry);
    KernelStart(kernel_info.base, &info);

    // Once boot services have been exited, must never return!
    while (1)
        HALT();

    return EFI_SUCCESS;
}
