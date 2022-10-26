/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#include <ahci.h>

#include <cstr.h>
#include <debug.h>
#include <integers.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_AHCI

namespace AHCI {
    const char* port_type_string(PortType p) {
        switch (p) {
        case PortType::None:
            return "None";
        case PortType::SATA:
            return "SATA";
        case PortType::SEMB:
            return "SEMB";
        case PortType::PM:
            return "PM";
        case PortType::SATAPI:
            return "SATAPI";
        default:
            return "Unknown Port Type";
        };
    }

    PortType get_port_type(HBAPort* port) {
        u32 sataStatus = port->SataStatus;
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

    PortController::PortController(PortType type, u64 portNumber
                                   , HBAPort* portAddress)
        : Type(type), PortNumber(portNumber), Port(portAddress)
    {
        // Get contiguous physical memory for
        // this AHCI port to read to/write from.
        Buffer = (u8*)Memory::request_pages(PORT_BUFFER_PAGES);
        // Wait for pending commands to finish, then stop any further commands.
        stop_commands();
        // Allocate memory for command list.
        void* base = Memory::request_page();
        memset(base, 0, 1024);
        Port->set_command_list_base(base);
        // Allocate memory for Frame Information Structure.
        void* fisBase = Memory::request_page();
        memset(fisBase, 0, 256);
        Port->set_frame_information_structure_base(fisBase);
        // Populate command list with command tables.
        auto* commandHeader = reinterpret_cast<HBACommandHeader*>(Port->command_list_base());
        for (u8 i = 0; i < 32; ++i) {
            // 8 PRDT entries per command table, aka 256 bytes.
            commandHeader[i].PRDTLength = 8;
            void* commandTableAddress = Memory::request_page();
            u64 address = reinterpret_cast<u64>(commandTableAddress) + (i << 8);
            commandHeader[i].set_command_table_base(address);
            memset(reinterpret_cast<void*>(address), 0, 256);
        }
        start_commands();

#ifdef DEBUG_AHCI
        dbgmsg("[AHCI]: Port %ull initialized.\r\n", PortNumber);
#endif /* DEBUG_AHCI */
    }

    bool PortController::read_low_level(u64 sector, u64 sectors) {
        // Ensure hardware port is not busy by
        // spinning until it isn't, or giving up.
        const u64 maxSpin = 1000000;
        u64 spin = 0;
        while ((Port->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < maxSpin)
            spin++;

        if (spin >= maxSpin)
            return false;

        // Disable interrupts during command construction.
        Port->InterruptStatus = (u32)-1;
        auto* commandHeader = reinterpret_cast<HBACommandHeader*>(Port->command_list_base());
        commandHeader->CommandFISLength = sizeof(FIS_REG_H2D)/sizeof(u32);
        commandHeader->Write = 0;
        commandHeader->PRDTLength = 1;

        auto* commandTable = reinterpret_cast<HBACommandTable*>(commandHeader->command_table_base());
        memset(commandTable, 0, sizeof(HBACommandTable) + ((commandHeader->PRDTLength - 1) * sizeof(HBA_PRDTEntry)));
        commandTable->PRDTEntry[0].set_data_base((u64)Buffer);
        commandTable->PRDTEntry[0].set_byte_count((sectors << 9) - 1);
        commandTable->PRDTEntry[0].set_interrupt_on_completion(true);
        FIS_REG_H2D* commandFIS = reinterpret_cast<FIS_REG_H2D*>(&commandTable->CommandFIS);
        commandFIS->Type = FIS_TYPE::REG_H2D;
        // Take control of command structure.
        commandFIS->CommandControl = 1;
        commandFIS->Command = ATA_CMD_READ_DMA_EXT;
        commandFIS->set_logical_block_addresses(sector);
        // Use lba mode.
        commandFIS->DeviceRegister = 1 << 6;
        // Set sector count.
        commandFIS->set_count(static_cast<u16>(sectors));
        // Issue command in first slot.
        Port->CommandIssue = 1;
        // Wait until command is completed.
        while (Port->CommandIssue != 0)
            if (Port->InterruptStatus & HBA_PxIS_TFES)
                return false;
        // Check once more after break that read did not fail.
        if (Port->InterruptStatus & HBA_PxIS_TFES)
            return false;
        
        return true;
    }

    /// Convert bytes to sectors, then read into and copy from intermediate
    /// `Buffer` to given `buffer` until all data is read and copied.
    void PortController::read(u64 byteOffset, u64 byteCount, u8* buffer) {
#ifdef DEBUG_AHCI
        dbgmsg("[AHCI]: Port %ull -- read()  byteOffset=%ull, byteCount=%ull, buffer=%x\r\n"
               , PortNumber
               , byteOffset
               , byteCount
               , buffer
               );
#endif /* DEBUG_AHCI */

        if (Type != PortType::SATA) {
            dbgmsg("  \033[31mERRROR\033[0m: `read()`  port type not implemented: %s\r\n"
                   , port_type_string(Type)
                   );
            return;
        }
        // TODO: Actual error handling!
        if (buffer == nullptr) {
            dbgmsg_s("  \033[31mERROR\033[0m: `read()`  buffer can not be nullptr\r\n");
            return;
        }
        // TODO: Don't reject reads over port buffer max size,
        //        just do multiple reads and copy as you go.
        if (byteCount > MAX_READ_BYTES) {
            dbgmsg_s("  \033[31mERROR\033[0m: `read()`  byteCount can not be larger than maximum readable bytes.\r\n");
            return;
        }

        u64 sector = byteOffset / BYTES_PER_SECTOR;
        u64 byteOffsetWithinSector = byteOffset % BYTES_PER_SECTOR;
        u64 sectors = (byteOffsetWithinSector + byteCount + BYTES_PER_SECTOR - 1) / BYTES_PER_SECTOR;

        if (byteOffsetWithinSector + byteCount <= BYTES_PER_SECTOR)
            sectors = 1;

#ifdef DEBUG_AHCI
        dbgmsg("  Calculated sector data: sector=%ull, sectors=%ull, byteOffsetWithinSector=%ull\r\n"
               , sector
               , sectors
               , byteOffsetWithinSector
               );
#endif /* DEBUG_AHCI */

        if (sectors * BYTES_PER_SECTOR > PORT_BUFFER_BYTES) {
            dbgmsg_s("  \033[31mERROR\033[0m: `read()`  can not read more bytes than internal buffer size.\r\n");
            return;
        }

        if (read_low_level(sector, sectors)) {
#ifdef DEBUG_AHCI
            dbgmsg_s("  \033[32mSUCCESS\033[0m: `read_low_level()` SUCCEEDED\r\n");
#endif /* DEBUG_AHCI */
            void* bufferAddress = (void*)((u64)&Buffer[0] + byteOffsetWithinSector);
            memcpy(bufferAddress, buffer, byteCount);
        }
        else dbgmsg_s("  \033[31mERROR\033[0m: `read_low_level()` FAILED\r\n");
    }

    void PortController::write(u64 byteOffset, u64 byteCount, u8* buffer) {
        dbgmsg("[AHCI]: TODO: Implement write()  byteOffset=%ull, byteCount=%ull, buffer=%x\r\n"
               , byteOffset
               , byteCount
               , buffer
               );
    }

    void PortController::start_commands() {
        while (Port->CommandAndStatus & HBA_PxCMD_CR);
        Port->CommandAndStatus |= HBA_PxCMD_FRE;
        Port->CommandAndStatus |= HBA_PxCMD_ST;
    }
    
    void PortController::stop_commands() {
        Port->CommandAndStatus &= ~HBA_PxCMD_ST;
        Port->CommandAndStatus &= ~HBA_PxCMD_FRE;
        while (Port->CommandAndStatus & HBA_PxCMD_FR
               && Port->CommandAndStatus & HBA_PxCMD_CR);
    }
}
