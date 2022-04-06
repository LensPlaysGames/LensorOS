#include "ahci.h"

#include "fat_definitions.h"
#include "gpt.h"
#include "inode.h"
#include "memory.h"
#include "memory/heap.h"
#include "memory/physical_memory_manager.h"
#include "memory/virtual_memory_manager.h"
#include "pci.h"
#include "smart_pointer.h"
#include "spinlock.h"
#include "system.h"

namespace AHCI {
    PortType get_port_type(HBAPort* port) {
        u32 sataStatus = port->sataStatus;
        u8 interfacePowerManagement = (sataStatus >> 8) & 0b111;
        u8 deviceDetection = sataStatus & 0b111;
        if (deviceDetection != HBA_PORT_DEVICE_PRESENT
            || interfacePowerManagement != HBA_PORT_IPM_ACTIVE)
        {
            // Device is not present or active.
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

    bool PortController::read_low_level(u64 sector, u64 sectors) {
            // Ensure hardware port is not busy by spinning until it isn't, or giving up.
            const u64 maxSpin = 1000000;
            u64 spin = 0;
            while ((Port->taskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < maxSpin) {
                spin++;
            }
            if (spin == maxSpin)
                return false;

            u32 sectorL = (u32)sector;
            u32 sectorH = (u32)(sector >> 32);
            // Disable interrupts during command construction.
            Port->interruptStatus = (u32)-1;
            auto* cmdHdr = (HBACommandHeader*)((u64)Port->commandListBase
                                               + ((u64)Port->commandListBaseUpper << 32));
            cmdHdr->commandFISLength = sizeof(FIS_REG_H2D)/sizeof(u32);
            cmdHdr->write = 0;
            cmdHdr->prdtLength = 1;
            auto* cmdTable = (HBACommandTable*)((u64)cmdHdr->commandTableBaseAddress
                                                + ((u64)cmdHdr->commandTableBaseAddressUpper << 32));
            memset(cmdTable, 0, sizeof(HBACommandTable) + ((cmdHdr->prdtLength-1) * sizeof(HBA_PRDTEntry)));
            cmdTable->prdtEntry[0].dataBaseAddress = (u32)((u64)Buffer);
            cmdTable->prdtEntry[0].dataBaseAddressUpper = (u32)((u64)Buffer >> 32);
            cmdTable->prdtEntry[0].byteCount = (sectors << 9) - 1;
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
            cmdFIS->countLow  = (sectors)      & 0xff;
            cmdFIS->countHigh = (sectors >> 8) & 0xff;
            // Issue command.
            Port->commandIssue = 1;
            // Wait until command is completed.
            while (Port->commandIssue != 0) {
                // I don't know why this is needed, but without
                //   this `nop` instruction, this loop never exits.
                asm volatile ("nop");
                if (Port->interruptStatus & HBA_PxIS_TFES)
                    return false;
            }
            // Check once more after break that read did not fail.
            if (Port->interruptStatus & HBA_PxIS_TFES)
                return false;
        
            return true;
    }
}
