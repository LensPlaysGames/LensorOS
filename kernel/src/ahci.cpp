#include <ahci.h>

#include <cstr.h>
#include <integers.h>
#include <uart.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_AHCI

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

    PortController::PortController(PortType type, u64 portNumber
                                   , HBAPort* portAddress)
        : Type(type), PortNumber(portNumber), Port(portAddress)
    {
        // Get contiguous physical memory for
        // this AHCI port to read to/write from.
        Buffer = (u8*)Memory::request_pages(PORT_BUFFER_PAGES);
        // Prevent interruptions while initializing port.
        stop_commands();
        // Allocate memory for command list.
        void* base = Memory::request_page();
        Port->commandListBase = (u32)(u64)base;
        Port->commandListBaseUpper = (u32)((u64)base >> 32);
        memset(base, 0, 1024);
        // Allocate memory for Frame Information Structure.
        void* fisBase = Memory::request_page();
        Port->fisBaseAddress = (u32)(u64)fisBase;
        Port->fisBaseAddressUpper = (u32)((u64)fisBase >> 32);
        memset(fisBase, 0, 256);
        // Populate command list with command tables.
        auto* cmdHdr = (HBACommandHeader*)((u64)Port->commandListBase
                                           + ((u64)Port->commandListBaseUpper << 32));
        for (u8 i = 0; i < 32; ++i) {
            cmdHdr[i].prdtLength = 8;
            void* cmdTableAddress = Memory::request_page();
            u64 address = (u64)cmdTableAddress + (i << 8);
            cmdHdr[i].commandTableBaseAddress = (u32)address;
            cmdHdr[i].commandTableBaseAddressUpper = (u32)((u64)address >> 32);
            memset(cmdTableAddress, 0, 256);
        }
        start_commands();

#ifdef DEBUG_AHCI
        UART::out("[AHCI]: Port ");
        UART::out(PortNumber);
        UART::out(" initialized\r\n");
#endif /* DEBUG_AHCI */
    }

    bool PortController::read_low_level(u64 sector, u64 sectors) {
        // Ensure hardware port is not busy by
        // spinning until it isn't, or giving up.
        const u64 maxSpin = 1000000;
        u64 spin = 0;
        while ((Port->taskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < maxSpin)
            spin++;

        if (spin >= maxSpin)
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

    /// Convert bytes to sectors, then read into and copy from intermediate
    /// `Buffer` to given `buffer` until all data is read and copied.
    void PortController::read(u64 byteOffset, u64 byteCount, u8* buffer) {
#ifdef DEBUG_AHCI
        UART::out("[AHCI]: Port ");
        UART::out(PortNumber);
        UART::out(" -- read()  byteOffset=");
        UART::out(byteOffset);
        UART::out(", byteCount=");
        UART::out(byteCount);
        UART::out(", buffer=0x");
        UART::out(to_hexstring(buffer));
        UART::out("\r\n");
#endif /* DEBUG_AHCI */

        // TODO: Actual error handling!
        if (buffer == nullptr) {
            UART::out("  \033[31mERROR\033[0m: `read()`  buffer can not be nullptr\r\n");
            return;
        }
        // TODO: Don't reject reads over port buffer max size,
        //        just do multiple reads and copy as you go.
        if (byteCount > MAX_READ_BYTES) {
            UART::out("  \033[31mERROR\033[0m: `read()`  byteCount can"
                      " not be larger than maximum readable bytes.\r\n");
            return;
        }

        u64 sector = byteOffset / BYTES_PER_SECTOR;
        u64 byteOffsetWithinSector = byteOffset % BYTES_PER_SECTOR;
        u64 sectors = (byteOffsetWithinSector + byteCount) / BYTES_PER_SECTOR;
        if (byteOffsetWithinSector + byteCount <= BYTES_PER_SECTOR)
            sectors = 1;

#ifdef DEBUG_AHCI
        UART::out("  Calculated sector data: sector=");
        UART::out(sector);
        UART::out(", sectors=");
        UART::out(sectors);
        UART::out("\r\n");
#endif /* DEBUG_AHCI */

        if (sectors * BYTES_PER_SECTOR > PORT_BUFFER_BYTES) {
            UART::out("  \033[31mERROR\033[0m: `read()`  can not read more bytes than internal buffer size.\r\n");
            return;
        }

        if (read_low_level(sector, sectors)) {
#ifdef DEBUG_AHCI
            UART::out("  \033[32mSUCCESS\033[0m: `read_low_level()` SUCCEEDED\r\n");
#endif /* DEBUG_AHCI */
            void* bufferAddress = (void*)((u64)&Buffer[0] + byteOffsetWithinSector);
            memcpy(bufferAddress, buffer, byteCount);
        }
        else UART::out("  \033[31mERROR\033[0m: `read_low_level()` FAILED\r\n");
    }

    void PortController::write(u64 byteOffset, u64 byteCount, u8* buffer) {
        UART::out("[TODO]: Implement write()  byteOffset=");
        UART::out(byteOffset);
        UART::out(", byteCount=");
        UART::out(byteCount);
        UART::out(", buffer=0x");
        UART::out(to_hexstring(buffer));
        UART::out("\r\n");
    }

    void PortController::start_commands() {
        // Spin until not busy.
        while (Port->cmdSts & HBA_PxCMD_CR);
        Port->cmdSts |= HBA_PxCMD_FRE;
        Port->cmdSts |= HBA_PxCMD_ST;
    }
    
    void PortController::stop_commands() {
        Port->cmdSts &= ~HBA_PxCMD_ST;
        Port->cmdSts &= ~HBA_PxCMD_FRE;
        while (Port->cmdSts & HBA_PxCMD_FR
               && Port->cmdSts & HBA_PxCMD_CR);
    }
}
