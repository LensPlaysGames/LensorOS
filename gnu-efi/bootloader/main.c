#include <efi.h>
#include <efilib.h>
#include <elf.h>

// Data Types
#include <stddef.h>
#include <stdint.h>

int memcmp (const void* aptr, const void* bptr, size_t n) {
	const unsigned char* a = aptr;
	const unsigned char* b = bptr;
	for (size_t i = 0; i < n; i++) {
		if (a[i] < b[i]) { return -1; }
		else if (a[i] > b[i]) { return 1; }
	}
	return 0;
}

EFI_FILE* LoadFile(EFI_FILE* dir, CHAR16* path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
	EFI_FILE* loadedFile;
	
	EFI_LOADED_IMAGE_PROTOCOL* loadedImage;
	SystemTable->BootServices->HandleProtocol(ImageHandle,
											  &gEfiLoadedImageProtocolGuid,
											  (void**)&loadedImage);
	
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystem;
	SystemTable->BootServices->HandleProtocol(loadedImage->DeviceHandle,
											  &gEfiSimpleFileSystemProtocolGuid,
											  (void**)&fileSystem);

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

PSF1_FONT* LoadPSF1Font(EFI_FILE* dir, CHAR16* path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
	EFI_FILE* font = LoadFile(dir, path, ImageHandle, SystemTable);
	if (font == NULL) { return NULL; }

	PSF1_HEADER* font_hdr;
	SystemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(PSF1_HEADER), (void**)&font_hdr);
	UINTN size = sizeof(PSF1_HEADER);
	font->Read(font, &size, font_hdr);

	if (font_hdr->Magic[0] != PSF1_MAGIC0
		|| font_hdr->Magic[1] != PSF1_MAGIC1)
	{
		Print(L"[ERR]: Invalid font format");
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
	SystemTable->BootServices->AllocatePool(EfiLoaderData, glyphBufferSize, (void**)&glyphBuffer);
	font->Read(font, &glyphBufferSize, glyphBuffer);

	PSF1_FONT* final_font;
	SystemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(PSF1_FONT), (void**)&final_font);
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
		Print(L"Unable to locate GOP\n");
		return NULL;
	}
	Print(L"[LOG]: GOP located successfully\n");

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
} BootInfo;

EFI_STATUS efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
	InitializeLib(ImageHandle, SystemTable);

	Print(L"LensorOS BOOTLOADER\n");

	// FIND KERNEL
	EFI_FILE* kernel = LoadFile(NULL, L"kernel.elf", ImageHandle, SystemTable);
	if (kernel == NULL) {
		Print(L"[ERR]: Could not load kernel\n");
		return 1;
	}
	Print(L"[LOG]: LensorOS kernel has been found\n");

	// LOAD KERNEL ELF64 HEADER INTO MEMORY
	Elf64_Ehdr elf_header;
	{
		UINTN fileInfoSize;
		EFI_FILE_INFO* fileInfo;

		// Get size of kernel.
		kernel->GetInfo(kernel, &gEfiFileInfoGuid, &fileInfoSize, NULL);
		// Allocate memory pool for kernel.
		SystemTable->BootServices->AllocatePool(EfiLoaderData,
												fileInfoSize,
												(void**)&fileInfo);
		// Fill memory with information from loaded kernel file.
		kernel->GetInfo(kernel, &gEfiFileInfoGuid,
						&fileInfoSize, (void**)&fileInfo);
		
		UINTN size = sizeof(elf_header);
		kernel->Read(kernel, &size, &elf_header);
	}

	// VERIFY KERNEL ELF64 HEADER
	if (memcmp(&elf_header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0
		|| elf_header.e_ident[EI_CLASS] != ELFCLASS64
		|| elf_header.e_ident[EI_DATA] != ELFDATA2LSB
		|| elf_header.e_type != ET_EXEC
		|| elf_header.e_machine != EM_X86_64
		|| elf_header.e_version != EV_CURRENT)
	{
		Print(L"[ERR]: Invalid kernel format\n");
		return 1;
	}
	Print(L"[LOG]: LensorOS kernel format verified successfully\n");

	// LOAD KERNEL INTO MEMORY
	Elf64_Phdr* program_hdrs;
	{
		kernel->SetPosition(kernel, elf_header.e_phoff);
		UINTN size = elf_header.e_phnum * elf_header.e_phentsize;
		SystemTable->BootServices->AllocatePool(EfiLoaderData, size, (void**)&program_hdrs);
		kernel->Read(kernel, &size, program_hdrs);
	}

	for (
		Elf64_Phdr* phdr = program_hdrs;
		(char*)phdr < (char*)program_hdrs + elf_header.e_phnum * elf_header.e_phentsize;
		phdr = (Elf64_Phdr*)((char*)phdr + elf_header.e_phentsize))
	{
		if (phdr->p_type == PT_LOAD) {
			// Allocate pages for program
			int pages = (phdr->p_memsz + 0x1000 - 1) / 0x1000;
			Elf64_Addr segment = phdr->p_paddr;
			SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);
			kernel->SetPosition(kernel, phdr->p_offset);
			UINTN size = phdr->p_filesz;
			kernel->Read(kernel, &size, (void*)segment);
		}
	}
	Print(L"[LOG]: LensorOS kernel loaded successfully\n");

	// Initialize Unified Extensible Firmware Interface Graphics Output Protocol.
	// Let's call that the 'GOP' from now on.
	Framebuffer* gop_fb = InitializeGOP();
	Print(L"UEFI GOP INFO:\n"
		  L"  Base: 0x%08X\n"
		  L"  Size: 0x%x\n"
		  L"  Pixel Width: %d\n"
		  L"  Pixel Height: %d\n"
		  L"  Pixels Per Scanline: %d\n",
		  gop_fb->BaseAddress, 
		  gop_fb->BufferSize, 
		  gop_fb->PixelWidth, 
		  gop_fb->PixelHeight, 
		  gop_fb->PixelsPerScanLine);

	// Read default font from root directory.
	PSF1_FONT* dflt_font = LoadPSF1Font(NULL, L"zap-vga16.psf", ImageHandle, SystemTable);
	if (dflt_font == NULL) {
		Print(L"[ERR]: Failed to load default font\n");
	}
	else {
		Print(L"[LOG]: Default font loaded successfully\n"
			  L"Default Font (PSF1) Info:"
			  L"  Character Size: 8x%d\n",
			  dflt_font->PSF1_Header->CharacterSize);
	}

	// EFI MEMORY MAP
	EFI_MEMORY_DESCRIPTOR* Map = NULL;
	UINTN MapSize, MapKey;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
	{
	  SystemTable->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
	  SystemTable->BootServices->AllocatePool(EfiLoaderData, MapSize, (void**)&Map);
	  SystemTable->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
	}

	BootInfo info;
	info.framebuffer = gop_fb;
	info.font = dflt_font;
	info.map = Map;
	info.mapSize = MapSize;
	info.mapDescSize = DescriptorSize;

	// Exit boot services: free system resources dedicated to UEFI boot services,
	//   as well as prevent UEFI from shutting down automatically after 5 minutes.
	SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);

	// Define kernel entry point.
	void (*KernelStart)(BootInfo*) = ((__attribute__((sysv_abi)) void (*)(BootInfo*)) elf_header.e_entry);
	KernelStart(&info);
	
	return EFI_SUCCESS;
}
