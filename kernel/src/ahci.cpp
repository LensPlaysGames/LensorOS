#include "ahci.h"

#include "fat_definitions.h"
#include "fat_driver.h"
#include "fat_fs.h"
#include "gpt.h"
#include "inode.h"
#include "memory.h"
#include "memory/heap.h"
#include "memory/physical_memory_manager.h"
#include "memory/virtual_memory_manager.h"
#include "pci.h"
#include "smart_pointer.h"
#include "spinlock.h"



namespace AHCI {
#define HBA_PORT_DEVICE_PRESENT 0x3
#define HBA_PORT_IPM_ACTIVE     0x1
#define HBA_PxCMD_CR   0x8000
#define HBA_PxCMD_FR   0x4000
#define HBA_PxCMD_FRE  0x10
#define HBA_PxCMD_ST   1

#define SATA_SIG_ATAPI 0xeb140101
#define SATA_SIG_ATA   0x00000101
#define SATA_SIG_SEMB  0xc33c0101
#define SATA_SIG_PM    0x96690101

    AHCIDriver** Drivers;
    u16 NumDrivers {0};

    PortType get_port_type(HBAPort* port) {
        u32 sataStatus = port->sataStatus;
        u8 interfacePowerManagement = (sataStatus >> 8) & 0b111;
        u8 deviceDetection = sataStatus & 0b111;

        if (deviceDetection != HBA_PORT_DEVICE_PRESENT
            || interfacePowerManagement != HBA_PORT_IPM_ACTIVE)
        {
            // Device is not present and/or active.
            return PortType::None;
        }

        switch (port->signature) {
        case SATA_SIG_ATAPI:
            return PortType::SATAPI;
        case SATA_SIG_ATA:
            return PortType::SATA;
        case SATA_SIG_SEMB:
            return PortType::SEMB;
        case SATA_SIG_PM:
            return PortType::PM;
        default:
            return PortType::None;
        }
    }

    void AHCIDriver::probe_ports() {
        u32 ports = ABAR->portsImplemented;
        for (u64 i = 0; i < 32; ++i) {
            if (ports & (1 << i)) {
                PortType type = get_port_type(&ABAR->ports[i]);
                if (type == PortType::SATA || type == PortType::SATAPI) {
                    Ports[NumPorts] = new Port(NumPorts
                                               , type
                                               , (u8*)Memory::request_pages(MAX_READ_PAGES)
                                               , &ABAR->ports[i]);
                    NumPorts++;
                }
            }
        }
    }

    void Port::initialize() {
        stop_commands();
        // Command Base
        void* base = Memory::request_page();
        HBAport->commandListBase = (u32)(u64)base;
        HBAport->commandListBaseUpper = (u32)((u64)base >> 32);
        memset(base, 0, 1024);
        // FIS Base
        void* fisBase = Memory::request_page();
        HBAport->fisBaseAddress = (u32)(u64)fisBase;
        HBAport->fisBaseAddressUpper = (u32)((u64)fisBase >> 32);
        memset(fisBase, 0, 256);
        HBACommandHeader* cmdHdr = (HBACommandHeader*)((u64)HBAport->commandListBase + ((u64)HBAport->commandListBaseUpper << 32));
        for (u64 i = 0; i < 32; ++i) {
            cmdHdr[i].prdtLength = 8;
            void* cmdTableAddress = Memory::request_page();
            u64 address = (u64)cmdTableAddress + (i << 8);
            cmdHdr[i].commandTableBaseAddress = (u32)address;
            cmdHdr[i].commandTableBaseAddressUpper = (u32)((u64)address >> 32);
            memset(cmdTableAddress, 0, 256);
        }
        start_commands();
    }


    void Port::start_commands() {
        // Spin until not busy.
        while (HBAport->cmdSts & HBA_PxCMD_CR);
        HBAport->cmdSts |= HBA_PxCMD_FRE;
        HBAport->cmdSts |= HBA_PxCMD_ST;
    }
    
    void Port::stop_commands() {
        HBAport->cmdSts &= ~HBA_PxCMD_ST;
        HBAport->cmdSts &= ~HBA_PxCMD_FRE;
        while (HBAport->cmdSts & HBA_PxCMD_FR
               && HBAport->cmdSts & HBA_PxCMD_CR);
    }

    // Do not use this directly, as buffer contents may be
    //   changed by another thread before accessing the buffer.
    bool Port::read_low_level(u64 sector, u16 numSectors) {
        // Ensure hardware port is not busy by spinning until it isn't, or giving up.
        const u64 maxSpin = 1000000;
        u64 spin = 0;
        while ((HBAport->taskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < maxSpin) {
            spin++;
        }
        if (spin == maxSpin)
            return false;

        u32 sectorL = (u32)sector;
        u32 sectorH = (u32)(sector >> 32);
        // Disable interrupts during command construction.
        HBAport->interruptStatus = (u32)-1;
        HBACommandHeader* cmdHdr = (HBACommandHeader*)(u64)HBAport->commandListBase;
        cmdHdr->commandFISLength = sizeof(FIS_REG_H2D)/sizeof(u32);
        cmdHdr->write = 0;
        cmdHdr->prdtLength = 1;
        HBACommandTable* cmdTable = (HBACommandTable*)(u64)cmdHdr->commandTableBaseAddress;
        memset(cmdTable, 0, sizeof(HBACommandTable) + ((cmdHdr->prdtLength-1) * sizeof(HBA_PRDTEntry)));
        cmdTable->prdtEntry[0].dataBaseAddress = (u32)((u64)Buffer);
        cmdTable->prdtEntry[0].dataBaseAddressUpper = (u32)((u64)Buffer >> 32);
        cmdTable->prdtEntry[0].byteCount = (numSectors << 9) - 1;
        cmdTable->prdtEntry[0].interruptOnCompletion = 1;
        FIS_REG_H2D* cmdFIS = (FIS_REG_H2D*)(&cmdTable->commandFIS);
        cmdFIS->type = FIS_TYPE::REG_H2D;
        // Take control of command
        cmdFIS->commandControl = 1;
        cmdFIS->command = ATA_CMD_READ_DMA_EX;
        // Assign lba's
        cmdFIS->lba0 = (u8)(sectorL);
        cmdFIS->lba1 = (u8)(sectorL >> 8);
        cmdFIS->lba2 = (u8)(sectorL >> 16);
        cmdFIS->lba3 = (u8)(sectorH);
        cmdFIS->lba4 = (u8)(sectorH >> 8);
        cmdFIS->lba5 = (u8)(sectorH >> 16);
        // Use lba mode.
        cmdFIS->deviceRegister = 1 << 6;
        // Set sector count.
        cmdFIS->countLow  = (numSectors)      & 0xff;
        cmdFIS->countHigh = (numSectors >> 8) & 0xff;
        // Issue command.
        HBAport->commandIssue = 1;
        // Wait until command is completed.
        while (HBAport->commandIssue != 0) {
            // I don't know why this is needed, but without
            //   this `nop` instruction, this loop never exits.
            asm volatile ("nop");
            if (HBAport->interruptStatus & HBA_PxIS_TFES)
                return false;
        }
        // Check once more after break that read did not fail.
        if (HBAport->interruptStatus & HBA_PxIS_TFES)
            return false;
        
        return true;
    }

    // Read a number of sectors starting at sector into port's physically-addressed buffer,
    //   then copy from the port's buffer into a given buffer without the possibility of the
    //   port's buffer being over-written by a read() call from a different thread.
    bool Port::read(u64 sector, u16 numSectors, void* dest, u64 numBytesToCopy) {
        // FIXME: Don't reject reads over port buffer max size,
        //        just do multiple reads and copy as you go.
        if (numBytesToCopy > MAX_READ_BYTES
            || dest == nullptr)
            return false;

        SpinlockLocker locker(Lock);
        bool status = read_low_level(sector, numSectors);
        if (status)
            memcpy(Buffer, dest, numBytesToCopy);
        return status;
    }

    AHCIDriver::AHCIDriver(PCI::PCIDeviceHeader* pciBaseAddress) {
        UART::out("\r\n[AHCI]: Constructing driver for AHCI 1.0 Controller at 0x");
        UART::out(to_hexstring(pciBaseAddress));
        UART::out("\r\n");
        
        PCIBaseAddress = pciBaseAddress;
        // ABAR = AHCI Base Address Register.
        ABAR = (HBAMemory*)(u64)(((PCI::PCIHeader0*)PCIBaseAddress)->BAR5);
        // Map ABAR into memory.
        Memory::map(ABAR, ABAR);

        UART::out("[AHCI]:\r\n  Mapping AHCI Base Memory Register (ABAR) to 0x");
        UART::out(to_hexstring(ABAR));
        UART::out("\r\n  Probing ABAR for open and active ports.\r\n");

        probe_ports();

        UART::out("  Found ");
        UART::out(NumPorts);
        UART::out(" open and active ports\r\n");
        UART::out("    Port read/write buffer size: ");
        UART::out(to_string(MAX_READ_PAGES * 4));
        UART::out("kib\r\n\r\n");

        for (u8 i = 0; i < NumPorts; ++i) {
            UART::out("[AHCI]: Port ");
            UART::out(to_string(i));
            UART::out("\r\n");
            Ports[i]->initialize();
            if (Ports[i]->Buffer == nullptr){
                UART::out("  \033[32mInitialization failed!\033[0m\r\n");
                continue;
            }
            memset((void*)Ports[i]->Buffer, 0, MAX_READ_BYTES);
            // Check if storage media at current port has a file-system LensorOS recognizes.
            // GPT (GUID Partition Table):
            if (GPT::is_gpt_present(Ports[i])) {
                UART::out("  Found Valid GPT:\r\n");
                auto hdr = SmartPtr<GPT::Header>(new GPT::Header);
                if (Ports[i]->read(1, 1, hdr.get(), sizeof(GPT::Header))) {
                    UART::out("    Partition Table Entries: ");
                    UART::out(to_string(hdr->NumberOfPartitionsTableEntries));
                    UART::out("\r\n");
                    auto sector = SmartPtr<u8[]>(new u8[512], 512);
                    for (u32 j = 0; j < hdr->NumberOfPartitionsTableEntries; ++j) {
                        u64 byteOffset = hdr->PartitionsTableEntrySize * j;
                        u32 partSector = hdr->PartitionsTableLBA + (byteOffset / 512);
                        byteOffset %= 512;
                        if (Ports[i]->read(partSector, 1, sector.get(), 512)) {
                            auto* part = (GPT::PartitionEntry*)((u64)sector.get() + byteOffset);
                            UART::out("      Partition ");
                            UART::out(to_string(j));
                            UART::out(": ");
                            UART::out(part->Name, 72);
                            UART::out(":\r\n        Type GUID: ");
                            print_guid(part->TypeGUID);
                            if (part->TypeGUID == GPT::PartitionType$EFISystem) {
                                UART::out("\r\n        Found EFI System partition (boot file system).");
                                // possible TODO: Mount boot filesystem as FAT32?
                            }
                            else {
                                for (GUID a : GPT::ReservedPartitionGUIDs) {
                                    if (a == part->TypeGUID) {
                                        UART::out("\r\n          This GUID matches a known partition type GUID, and will be preserved and ignored.");
                                        break;
                                    }
                                }
                            }
                            UART::out("\r\n        Unique GUID: ");
                            print_guid(part->UniqueGUID);
                            UART::out("\r\n        Size in Sectors: ");
                            UART::out(to_string(part->EndLBA - part->StartLBA));
                            UART::out("\r\n        Attributes: ");
                            UART::out(to_string(part->Attributes));
                            UART::out("\r\n");
                        }
                    }
                }
            }
            // FAT (File Allocation Table):
            else if (gFATDriver.is_device_fat_formatted(Ports[i])) {
                FatFS* fs = new FatFS(NumFileSystems, this, i);
                FileSystems[NumFileSystems] = fs;
                ++NumFileSystems;

                // FIXME: Dummy inode creation.
                Inode inode = Inode(*FileSystems[NumFileSystems], 0);
                fs->read(&inode);

                switch (fs->Type) {
                case FATType::INVALID: 
                    UART::out("  \033[31mINVALID\033[0m FAT format.");
                    break;
                case FATType::FAT32:   
                    UART::out("  FAT32 formatted");
                    break;
                case FATType::FAT16:   
                    UART::out("  FAT16 formatted");
                    break;
                case FATType::FAT12:   
                    UART::out("  FAT12 formatted");
                    break;
                case FATType::ExFAT:   
                    UART::out("  ExFAT formatted");
                    break;
                }
                UART::out("\r\n");

                // Write label and type of FAT device.
                switch (fs->Type) {
                case FATType::FAT12:
                case FATType::FAT16:
                    UART::out("  Label: ");
                    UART::out(&((BootRecordExtension16*)&fs->BR.Extended)->VolumeLabel[0], 11);
                    UART::out("\r\n");
                    break;
                case FATType::FAT32:
                case FATType::ExFAT:
                    UART::out("  Label: ");
                    UART::out(&((BootRecordExtension32*)&fs->BR.Extended)->VolumeLabel[0], 11);
                    UART::out("\r\n");
                    break;
                default:
                    break;
                }
                UART::out("  Total Size: ");
                UART::out(to_string(fs->get_total_size() / 1024 / 1024));
                UART::out(" mib\r\n");
            }
            else UART::out("  \033[31mUnrecognizable format.\033[0m\r\n");
            UART::out("\r\n");
        }
        UART::out("[AHCI]: \033[32mDriver constructed.\033[0m\r\n\r\n");
    }
    
    AHCIDriver::~AHCIDriver() {
        UART::out("[AHCI]: Deconstructing AHCI Driver\r\n");
        for(u32 i = 0; i < NumPorts; ++i) {
            Memory::free_pages((void*)Ports[i]->Buffer, MAX_READ_PAGES);
            delete Ports[i];
        }
    }
}
