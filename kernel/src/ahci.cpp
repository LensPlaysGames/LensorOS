#include "ahci.h"
#include "FATDriver.h"
#include "FatFS.h"
#include "FAT_definitions.h"
#include "vfs_inode.h"

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
        case SATA_SIG_ATAPI: { return PortType::SATAPI; }
        case SATA_SIG_ATA:   { return PortType::SATA;   }
        case SATA_SIG_SEMB:  { return PortType::SEMB;   }
        case SATA_SIG_PM:    { return PortType::PM;     }
        default:             { return PortType::None;   }
        }
    }

    void AHCIDriver::probe_ports() {
        u32 ports = ABAR->portsImplemented;
        for (u64 i = 0; i < 32; ++i) {
            if (ports & (1 << i)) {
                PortType type = get_port_type(&ABAR->ports[i]);
                if (type == PortType::SATA
                    || type == PortType::SATAPI)
                {
                    Ports[numPorts] = new Port;
                    Ports[numPorts]->hbaPort = &ABAR->ports[i];
                    Ports[numPorts]->type = type;
                    Ports[numPorts]->number = numPorts;
                    numPorts++;
                }
            }
        }
    }

    void Port::Configure() {
        StopCMD();
        // Command Base
        void* base = gAlloc.request_page();
        hbaPort->commandListBase = (u32)(u64)base;
        hbaPort->commandListBaseUpper = (u32)((u64)base >> 32);
        memset(base, 0, 1024);
        // FIS Base
        void* fisBase = gAlloc.request_page();
        hbaPort->fisBaseAddress = (u32)(u64)fisBase;
        hbaPort->fisBaseAddressUpper = (u32)((u64)fisBase >> 32);
        memset(fisBase, 0, 256);

        HBACommandHeader* cmdHdr = (HBACommandHeader*)((u64)hbaPort->commandListBase + ((u64)hbaPort->commandListBaseUpper << 32));
        for (u64 i = 0; i < 32; ++i) {
            cmdHdr[i].prdtLength = 8;
            void* cmdTableAddress = gAlloc.request_page();
            u64 address = (u64)cmdTableAddress + (i << 8);
            cmdHdr[i].commandTableBaseAddress = (u32)address;
            cmdHdr[i].commandTableBaseAddressUpper = (u32)((u64)address >> 32);
            memset(cmdTableAddress, 0, 256);
        }
        
        StartCMD();
    }


    void Port::StartCMD() {
        // Spin until not busy.
        while (hbaPort->cmdSts & HBA_PxCMD_CR);
        hbaPort->cmdSts |= HBA_PxCMD_FRE;
        hbaPort->cmdSts |= HBA_PxCMD_ST;
    }
    
    void Port::StopCMD() {
        hbaPort->cmdSts &= ~HBA_PxCMD_ST;
        hbaPort->cmdSts &= ~HBA_PxCMD_FRE;
        while (hbaPort->cmdSts & HBA_PxCMD_FR
               && hbaPort->cmdSts & HBA_PxCMD_CR);
    }

    bool Port::Read(u64 sector, u16 numSectors, void* buffer) {
        const u64 maxSpin = 1000000;
        u64 spin = 0;
        while ((hbaPort->taskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ))
               && spin < maxSpin)
        {
            spin++;
        }
        if (spin == maxSpin) {
            return false;
        }

        u32 sectorL = (u32)sector;
        u32 sectorH = (u32)(sector >> 32);

        hbaPort->interruptStatus = (u32)-1;

        HBACommandHeader* cmdHdr = (HBACommandHeader*)(u64)hbaPort->commandListBase;
        cmdHdr->commandFISLength = sizeof(FIS_REG_H2D)/sizeof(u32);
        cmdHdr->write = 0;
        cmdHdr->prdtLength = 1;
        HBACommandTable* cmdTable = (HBACommandTable*)(u64)cmdHdr->commandTableBaseAddress;
        memset(cmdTable, 0, sizeof(HBACommandTable) + ((cmdHdr->prdtLength-1) * sizeof(HBA_PRDTEntry)));
        cmdTable->prdtEntry[0].dataBaseAddress = (u32)(u64)buffer;
        cmdTable->prdtEntry[0].dataBaseAddressUpper = (u32)((u64)buffer >> 32);
        cmdTable->prdtEntry[0].byteCount = (numSectors << 9) - 1;
        cmdTable->prdtEntry[0].interruptOnCompletion = 1;
        FIS_REG_H2D* cmdFIS = (FIS_REG_H2D*)(&cmdTable->commandFIS);
        cmdFIS->type = FIS_TYPE::REG_H2D;
        // take control of command
        cmdFIS->commandControl = 1;
        cmdFIS->command = ATA_CMD_READ_DMA_EX;
        // assign lba's
        cmdFIS->lba0 = (u8)(sectorL);
        cmdFIS->lba1 = (u8)(sectorL >> 8);
        cmdFIS->lba2 = (u8)(sectorL >> 16);
        cmdFIS->lba3 = (u8)(sectorH);
        cmdFIS->lba4 = (u8)(sectorH >> 8);
        cmdFIS->lba5 = (u8)(sectorH >> 16);
        // use lba mode.
        cmdFIS->deviceRegister = 1 << 6;
        // set sector count.
        cmdFIS->countLow  = (numSectors)      & 0xff;
        cmdFIS->countHigh = (numSectors >> 8) & 0xff;
        // issue command.
        hbaPort->commandIssue = 1;
        // Wait until command is completed.
        while (true) {
            if (hbaPort->commandIssue == 0) { break; }
            if (hbaPort->interruptStatus & HBA_PxIS_TFES) {
                return false;
            }
        }
        // Check once more after break that read did not fail.
        if (hbaPort->interruptStatus & HBA_PxIS_TFES) {
            return false;
        }
        return true;
    }

    AHCIDriver::AHCIDriver(PCI::PCIDeviceHeader* pciBaseAddress) {
        srl.writestr("[AHCI]: Constructing driver.\r\n");

        PCIBaseAddress = pciBaseAddress;
        
        ABAR = (HBAMemory*)(u64)(((PCI::PCIHeader0*)PCIBaseAddress)->BAR5);
        // Map ABAR into memory.
        gPTM.map_memory(ABAR, ABAR);
        
        srl.writestr("[AHCI]: Probing AHCI 1.0 Controller at ");
        srl.writestr(to_hexstring((u64)PCIBaseAddress));
        srl.writestr("\r\n");

        // Probe ABAR for port info.
        probe_ports();
        
        srl.writestr("[AHCI]: Found ");
        srl.writestr(to_string((u64)numPorts));
        srl.writestr(" open and active ports\r\n");
        srl.writestr("[AHCI]: Read/write buffer size: ");
        srl.writestr(to_string((u64)MAX_READ_PAGES * 4));
        srl.writestr("kib\r\n");

        for (u8 i = 0; i < numPorts; ++i) {
            Ports[i]->Configure();
            Ports[i]->buffer = (u8*)gAlloc.request_pages(MAX_READ_PAGES);
            if (Ports[i]->buffer != nullptr) {
                srl.writestr("[AHCI]: Port ");
                srl.writestr(to_string((u64)i));
                srl.writestr(" configured successfully.\r\n");
                memset((void*)Ports[i]->buffer, 0, MAX_READ_PAGES * 0x1000);
                // Check if storage media at current port has a file-system LensorOS recognizes.
                // FAT (File Allocation Table):
                if (gFATDriver.is_device_fat_formatted(this, i)) {
                    // TODO: Cache file-system for later use.
                    FatFS* fs = new FatFS(NumFileSystems, this, i);
                    FileSystems[NumFileSystems] = fs;
                    ++NumFileSystems;

                    Inode inode = Inode();
                    fs->read(&inode);

                    srl.writestr("[AHCI]: Device at port ");
                    srl.writestr(to_string((u64)i));
                    switch (fs->Type) {
                    case FATType::INVALID: 
                        srl.writestr(" has INVALID format.");
                        break;
                    case FATType::FAT32:   
                        srl.writestr(" is FAT32 formatted.");
                        break;
                    case FATType::FAT16:   
                        srl.writestr(" is FAT16 formatted.");
                        break;
                    case FATType::FAT12:   
                        srl.writestr(" is FAT12 formatted.");
                        break;
                    case FATType::ExFAT:   
                        srl.writestr(" is ExFAT formatted.");
                        break;
                    }
                    srl.writestr("\r\n");

                    // Write label and type of FAT device.
                    switch (fs->Type) {
                    case FATType::FAT12:
                    case FATType::FAT16:
                        srl.writestr("  Label: ");
                        srl.writestr((char*)&((BootRecordExtension16*)&fs->BR.Extended)->VolumeLabel[0], 11);
                        srl.writestr("\r\n");
                        break;
                    case FATType::FAT32:
                    case FATType::ExFAT:
                        srl.writestr("  Label: ");
                        srl.writestr((char*)&((BootRecordExtension32*)&fs->BR.Extended)->VolumeLabel[0], 11);
                        srl.writestr("\r\n");
                        break;
                    default:
                        break;
                    }
                    srl.writestr("  Total Size: ");
                    srl.writestr(to_string((u64)fs->get_total_size() / 1024 / 1024));
                    srl.writestr(" mib\r\n");
                }
            }
        }
        srl.writestr("[AHCI]: Driver constructed.\r\n");
    }
    
    AHCIDriver::~AHCIDriver() {
        srl.writestr("[AHCI]: Deconstructing AHCI Driver\r\n");
        for(u32 i = 0; i < numPorts; ++i) {
            gAlloc.free_pages((void*)Ports[i]->buffer, MAX_READ_PAGES);
            delete Ports[i];
        }
    }
}
