OSNAME = LensorOS

GNUEFI = ../gnu-efi
OVMFDIR = ../OVMFbin
LDS = kernel.ld
ASMC = nasm
CC = gcc
LD = ld

ASMFLAGS = -f elf64
CFLAGS = -ffreestanding -fno-rtti -fshort-wchar -mno-red-zone -fno-exceptions -Wall
LDFLAGS = -T $(LDS) -static -Bsymbolic -nostdlib

SRCDIR := src
OBJDIR := lib
BUILDDIR = bin
BOOTEFI := $(GNUEFI)/x86_64/bootloader/main.efi

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC = $(call rwildcard,$(SRCDIR),*.cpp)
ASMSRC = $(call rwildcard,$(SRCDIR),*.asm)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
OBJS += $(patsubst $(SRCDIR)/%.asm, $(OBJDIR)/%_asm.o, $(ASMSRC))
DIRS = $(wildcard $(SRCDIR)/*)

.PHONY: default
default: all;

setup:
	@mkdir $(BUILDDIR)
	@mkdir $(OBJDIR)

clean:
	rm -r $(OBJDIR)

image:
	@echo "\n!===--- Creating FAT32 formatted bootable image ---===!\n"
	dd if=/dev/zero of=$(BUILDDIR)/$(OSNAME).img count=93750
	mformat -i $(BUILDDIR)/$(OSNAME).img -F ::
	mmd -i $(BUILDDIR)/$(OSNAME).img ::/EFI
	mmd -i $(BUILDDIR)/$(OSNAME).img ::/EFI/BOOT
	mmd -i $(BUILDDIR)/$(OSNAME).img ::/LensorOS
	mcopy -i $(BUILDDIR)/$(OSNAME).img $(BOOTEFI) ::/EFI/BOOT
	mcopy -i $(BUILDDIR)/$(OSNAME).img startup.nsh ::
	mcopy -i $(BUILDDIR)/$(OSNAME).img $(BUILDDIR)/kernel.elf ::/LensorOS
	mcopy -i $(BUILDDIR)/$(OSNAME).img $(BUILDDIR)/dfltfont.psf ::/LensorOS

fat12_image:
	@echo "\n!===--- Creating FAT12 formatted bootable image ---===!\n"
	dd if=/dev/zero of=$(BUILDDIR)/$(OSNAME).img count=93750
	mformat -i $(BUILDDIR)/$(OSNAME).img -f 1440 ::
	mmd -i $(BUILDDIR)/$(OSNAME).img ::/EFI
	mmd -i $(BUILDDIR)/$(OSNAME).img ::/EFI/BOOT
	mmd -i $(BUILDDIR)/$(OSNAME).img ::/LensorOS
	mcopy -i $(BUILDDIR)/$(OSNAME).img $(BOOTEFI) ::/EFI/BOOT
	mcopy -i $(BUILDDIR)/$(OSNAME).img startup.nsh ::
	mcopy -i $(BUILDDIR)/$(OSNAME).img $(BUILDDIR)/kernel.elf ::/LensorOS
	mcopy -i $(BUILDDIR)/$(OSNAME).img $(BUILDDIR)/dfltfont.psf ::/LensorOS

kernel: $(OBJS) link
$(OBJDIR)/interrupts/interrupts.o: $(SRCDIR)/interrupts/interrupts.cpp
	@mkdir -p $(@D)
	$(CC) -mno-red-zone -mgeneral-regs-only -ffreestanding -fno-exceptions -c $^ -o $@
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@
$(OBJDIR)/%_asm.o: $(SRCDIR)/%.asm
	@mkdir -p $(@D)
	$(ASMC) $(ASMFLAGS) $^ -o $@

kernel_debug: $(OBJS) link
$(OBJDIR)/interrupts/interrupts.o: $(SRCDIR)/interrupts/interrupts.cpp
	@mkdir -p $(@D)
	$(CC) -g -mno-red-zone -mgeneral-regs-only -ffreestanding -fno-exceptions -c $^ -o $@
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CC) -g $(CFLAGS) -c $^ -o $@
$(OBJDIR)/%_asm.o: $(SRCDIR)/%.asm
	@mkdir -p $(@D)
	$(ASMC) -g $(ASMFLAGS) $^ -o $@

link:
	@echo "\n!===--- Linking kernel ---===!\n"
	$(LD) $(LDFLAGS) -o $(BUILDDIR)/kernel.elf $(OBJS)

rekernel:
	$(MAKE) clean
	$(MAKE) kernel

all:
	$(MAKE) rekernel
	$(MAKE) image

run:
	@echo "\n!===--- Running QEMU ---===!\n"
	qemu-system-x86_64 -cpu qemu64 -m 100M -rtc base=localtime,clock=host,driftfix=none -machine q35 -serial stdio -vga cirrus -drive format=raw,file=$(BUILDDIR)/$(OSNAME).img -drive if=pflash,format=raw,unit=0,file="$(OVMFDIR)/OVMF_CODE-pure-efi.fd",readonly=on -drive if=pflash,format=raw,unit=1,file="$(OVMFDIR)/OVMF_VARS-pure-efi.fd" -net none
