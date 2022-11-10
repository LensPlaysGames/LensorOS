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

#include <format>

#include <ahci.h>

#include <cstr.h>
#include <debug.h>
#include <integers.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_AHCI

#ifdef DEBUG_AHCI
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...) void()
#endif

namespace AHCI {
    const char* port_type_strings[5] = {
        "None",
        "SATA",
        "SEMB",
        "PM",
        "SATAPI"
    };
    const char* port_type_string(PortType p) {
        return port_type_strings[p];
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

        DBGMSG("[AHCI]: Port {} initialized.\n", PortNumber);
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
        auto* commandFIS = reinterpret_cast<FIS_REG_H2D*>(&commandTable->CommandFIS);
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
    ssz PortController::read(usz byteOffset, usz byteCount, void* buffer) {
        DBGMSG("[AHCI]: Port {} -- read()  byteOffset={}, byteCount={}, buffer={}\n"
               , PortNumber
               , byteOffset
               , byteCount
               , (void*) buffer
               );

        if (Type != PortType::SATA) {
            std::print("  \033[31mERRROR\033[0m: `read()`  port type not implemented: {}\n"
                       , port_type_string(Type));
            return -1;
        }
        // TODO: Actual error handling!
        if (buffer == nullptr) {
            std::print("  \033[31mERROR\033[0m: `read()`  buffer can not be nullptr\n");
            return -1;
        }
        // TODO: Don't reject reads over port buffer max size, just do multiple reads and copy as you go.
        if (byteCount > MAX_READ_BYTES) {
            std::print("  \033[31mERROR\033[0m: `read()`  byteCount can not be larger than maximum readable bytes.\n");
            return -1;
        }

        u64 sector = byteOffset / BYTES_PER_SECTOR;
        u64 byteOffsetWithinSector = byteOffset % BYTES_PER_SECTOR;
        u64 sectors = (byteOffsetWithinSector + byteCount + BYTES_PER_SECTOR - 1) / BYTES_PER_SECTOR;

        if (byteOffsetWithinSector + byteCount <= BYTES_PER_SECTOR)
            sectors = 1;

        DBGMSG("  Calculated sector data: sector={}, sectors={}, byteOffsetWithinSector={}\n"
               , sector
               , sectors
               , byteOffsetWithinSector
               );

        if (sectors * BYTES_PER_SECTOR > PORT_BUFFER_BYTES) {
            DBGMSG("  \033[31mERROR\033[0m: `read()`  can not read more bytes than internal buffer size.\n");
            return -1;
        }

        if (read_low_level(sector, sectors)) {
            DBGMSG("  \033[32mSUCCESS\033[0m: `read_low_level()` SUCCEEDED\n");
            void* bufferAddress = (void*)((u64)&Buffer[0] + byteOffsetWithinSector);
            memcpy(buffer, bufferAddress, byteCount);
        } else DBGMSG("  \033[31mERROR\033[0m: `read_low_level()` FAILED\n");

        return byteCount;
    }

    ssz PortController::write(usz byteOffset, usz byteCount, void* buffer) {
        std::print("[AHCI]: TODO: Implement write()  byteOffset={}, byteCount={}, buffer={}\n"
                   , byteOffset
                   , byteCount
                   , (void*) buffer
                   );
        return -1;
    }

    _PushIgnoreWarning("-Wvolatile")
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
    _PopWarnings()
}
