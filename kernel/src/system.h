#ifndef LENSOR_OS_SYSTEM_H
#define LENSOR_OS_SYSTEM_H

#include "cpu.h"
#include "file.h"
#include "guid.h"
#include "integers.h"
#include "linked_list.h"
#include "memory/physical_memory_manager.h"
#include "pure_virtuals.h"

/* TODO:
 * |-- What to do about duplicate devices? count? unique id?
 * `-- Keep track of clock sources (either through system devices or otherwise)
 */

/*     Storage Dev. Driver                        
 *     |-- read(SectorOffset,SectorCount,Buffer)  
 *     `-- write(SectorOffset,SectorCount,Buffer) 
 *
 *     Filesystem Driver
 *     |-- read(Storage Dev. Driver Ptr,Path,Buffer,Count)
 *     `-- write(Storage Dev. Driver Ptr,Path,Buffer,Count)
 *
 *     Filesystem Driver calls Storage Device Driver
 */

/* System Device Number:
 * |-- 0: Reserved
 * `-- 1: Storage Device -- StorageDeviceDriver at Data1
 *     |-- 0: AHCI Controller
 *     |   `-- Data2: PCI Device Header Address
 *     |-- 1: AHCI Port
 *     |   `-- Data2: AHCI Controller System Device Ptr
 *     `-- 10: GPT Partition
 *         |-- Data2: Partition Data (Sector offset, total size)
 *         `-- Data4: System Device Ptr (To storage device this part. resides on)
 */

constexpr u64 SYSDEV_MAJOR_STORAGE = 1;
constexpr u64 SYSDEV_MINOR_AHCI_CONTROLLER = 0;
constexpr u64 SYSDEV_MINOR_AHCI_PORT = 1;
constexpr u64 SYSDEV_MINOR_GPT_PARTITION = 10;

/// A system device refers to a hardware device that has been detected
/// during boot, and is saved for later use in the system structure.
class SystemDevice {
public:
    SystemDevice(u64 major, u64 minor)
        : Major(major), Minor(minor)
    {
        Data1 = nullptr;
        Data2 = nullptr;
        Data3 = nullptr;
        Data4 = nullptr;
    };

    SystemDevice(u64 major, u64 minor, void* data)
        : Major(major), Minor(minor), Data1(data)
    {
        Data2 = nullptr;
        Data3 = nullptr;
        Data4 = nullptr;
    };

    SystemDevice(u64 major, u64 minor
                 , void* data1, void* data2
                 , void* data3, void* data4)
        : Major(major), Minor(minor)
        , Data1(data1), Data2(data2)
        , Data3(data3), Data4(data4) {};

    u64 major() { return Major; }
    u64 minor() { return Minor; }
    void* data1() { return Data1; }
    void* data2() { return Data2; }
    void* data3() { return Data3; }
    void* data4() { return Data4; }

private:
    u64 Major { 0 };
    u64 Minor { 0 };
    void* Data1 { nullptr };
    void* Data2 { nullptr };
    void* Data3 { nullptr };
    void* Data4 { nullptr };
};

class StorageDeviceDriver {
public:
    virtual void read(u64 byteOffset, u64 byteCount, u8* buffer) = 0;
    virtual void write(u64 byteOffset, u64 byteCount, u8* buffer) = 0;
};

class GPTPartitionDriver final : public StorageDeviceDriver {
public:
    GPTPartitionDriver(StorageDeviceDriver* driver
                       , GUID type, GUID unique
                       , u64 startSector, u64 sectorSize)
        : Driver(driver)
        , Type(type), Unique(unique)
        , Offset(startSector * sectorSize) {}

    virtual void read(u64 byteOffset, u64 byteCount, u8* buffer) override {
        Driver->read(byteOffset + Offset, byteCount, buffer);
    };

    virtual void write(u64 byteOffset, u64 byteCount, u8* buffer) override {
        Driver->read(byteOffset + Offset, byteCount, buffer);
    };

    GUID type_guid() { return Type; }
    GUID unique_guid() { return Unique; }

private:
    StorageDeviceDriver* Driver { nullptr };
    GUID Type;
    GUID Unique;
    /// Number of bytes to offset within storage device for start of partition.
    u64 Offset { 0 };
};

namespace AHCI {
    // TODO: Initialize HBA port fields (allocations for CmdHdr, etc.)
    class PortController final : public StorageDeviceDriver {
    public:
        PortController(PortType type, u64 portNumber, HBAPort* portAddress)
            : Type(type), PortNumber(portNumber), Port(portAddress)
        {
            // Get contiguous physical memory for this AHCI port to read to/write from.
            Buffer = (u8*)Memory::request_pages(PORT_BUFFER_PAGES);

            stop_commands();
            // Command Base
            void* base = Memory::request_page();
            Port->commandListBase = (u32)(u64)base;
            Port->commandListBaseUpper = (u32)((u64)base >> 32);
            memset(base, 0, 1024);
            // FIS Base
            void* fisBase = Memory::request_page();
            Port->fisBaseAddress = (u32)(u64)fisBase;
            Port->fisBaseAddressUpper = (u32)((u64)fisBase >> 32);
            memset(fisBase, 0, 256);
            HBACommandHeader* cmdHdr = (HBACommandHeader*)((u64)Port->commandListBase
                                                           + ((u64)Port->commandListBaseUpper << 32));
            for (u64 i = 0; i < 32; ++i) {
                cmdHdr[i].prdtLength = 8;
                void* cmdTableAddress = Memory::request_page();
                u64 address = (u64)cmdTableAddress + (i << 8);
                cmdHdr[i].commandTableBaseAddress = (u32)address;
                cmdHdr[i].commandTableBaseAddressUpper = (u32)((u64)address >> 32);
                memset(cmdTableAddress, 0, 256);
            }
            start_commands();

            UART::out("[AHCI]: Port ");
            UART::out(PortNumber);
            UART::out(" initialized\r\n");
        }

        /// Populate `Buffer` with `sectors` amount of data starting at `sector`.
        bool read_low_level(u64 sector, u64 sectors) {
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

        /// Convert bytes to sectors, then read into and copy from intermediate
        /// `Buffer` to given `buffer` until all data is read and copied.
        virtual void read(u64 byteOffset, u64 byteCount, u8* buffer) override {
            UART::out("[AHCI]: Port ");
            UART::out(PortNumber);
            UART::out(" -- read()  byteOffset=");
            UART::out(byteOffset);
            UART::out(", byteCount=");
            UART::out(byteCount);
            UART::out(", buffer=0x");
            UART::out(to_hexstring(buffer));
            UART::out("\r\n");

            // TODO: Actual error handling!
            if (buffer == nullptr) {
                UART::out("  \033[31mERROR\033[0m: `read()`  buffer can not be nullptr\r\n");
                return;
            }
            // FIXME: Don't reject reads over port buffer max size,
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

            UART::out("  Calculated sector data: sector=");
            UART::out(sector);
            UART::out(", sectors=");
            UART::out(sectors);
            UART::out("\r\n");

            if (sectors * BYTES_PER_SECTOR > PORT_BUFFER_BYTES) {
                UART::out("  \033[31mERROR\033[0m: `read()`  can not read more bytes than internal buffer size.\r\n");
                return;
            }

            if (read_low_level(sector, sectors)) {
                UART::out("  \033[32mSUCCESS\033[0m: `read_low_level()` SUCCEEDED\r\n");
                void* bufferAddress = (void*)((u64)&Buffer[0] + byteOffsetWithinSector);
                memcpy(bufferAddress, buffer, byteCount);
            }
            else UART::out("  \033[31mERROR\033[0m: `read_low_level()` FAILED\r\n");
        };

        virtual void write(u64 byteOffset, u64 byteCount, u8* buffer) override {
            UART::out("[TODO]: Implement write()  byteOffset=");
            UART::out(byteOffset);
            UART::out(", byteCount=");
            UART::out(byteCount);
            UART::out(", buffer=0x");
            UART::out(to_hexstring(buffer));
            UART::out("\r\n");
        };

        u64 port_number() { return PortNumber; }

    private:
        PortType Type { PortType::None };
        u64 PortNumber { 99 };
        HBAPort* Port { nullptr };
        u8* Buffer { nullptr };
        const u64 BYTES_PER_SECTOR = 512;
        const u64 PORT_BUFFER_PAGES = 0x100;
        const u64 PORT_BUFFER_BYTES = PORT_BUFFER_PAGES * 0x1000;

#define HBA_PORT_DEVICE_PRESENT 0x3
#define HBA_PORT_IPM_ACTIVE     0x1
#define HBA_PxCMD_CR   0x8000
#define HBA_PxCMD_FR   0x4000
#define HBA_PxCMD_FRE  0x10
#define HBA_PxCMD_ST   1

        void start_commands() {
            // Spin until not busy.
            while (Port->cmdSts & HBA_PxCMD_CR);
            Port->cmdSts |= HBA_PxCMD_FRE;
            Port->cmdSts |= HBA_PxCMD_ST;
        }
    
        void stop_commands() {
            Port->cmdSts &= ~HBA_PxCMD_ST;
            Port->cmdSts &= ~HBA_PxCMD_FRE;
            while (Port->cmdSts & HBA_PxCMD_FR
                   && Port->cmdSts & HBA_PxCMD_CR);
        }
    };
}

class FilesystemDriver {
public:
    virtual void read(StorageDeviceDriver* driver
                      , const char* path
                      , void* buffer, u64 numBytes) = 0;
    virtual void write(StorageDeviceDriver* driver
                       , const char* path
                       , void* buffer, u64 numBytes) = 0;
};

enum class FilesystemType {
    INVALID = 0,
    FAT = 1,
};

static const char* fstype_to_string(FilesystemType t) {
    switch (t) {
    case FilesystemType::INVALID:
        return "Invalid";
    case FilesystemType::FAT:
        return "File Allocation Table";
    default:
        return "Unknown filesystem type";
    };
};

class Filesystem {
public:
    Filesystem(FilesystemType t
               , FilesystemDriver* fs
               , StorageDeviceDriver* dev)
        : Type(t), FSDriver(fs), DevDriver(dev) {}

    FilesystemType type() { return Type; }
    FilesystemDriver* filesystem_driver() { return FSDriver; }
    StorageDeviceDriver* storage_device_driver() { return DevDriver; }

    void read(const char* path, void* buffer, u64 numBytes) {
        FSDriver->read(DevDriver, path, buffer, numBytes);
    };

    void write(const char* path, void* buffer, u64 numBytes) {
        FSDriver->write(DevDriver, path, buffer, numBytes);
    };

private:
    FilesystemType Type; // This doesn't do much right now.
    FilesystemDriver* FSDriver { nullptr };
    StorageDeviceDriver* DevDriver { nullptr };
};

class System {
public:
    System() {}

    void set_cpu(const CPUDescription& cpu) {
        CPU = cpu;
    }

    void add_device(SystemDevice d) {
        Devices.add(d);
    };

    void add_fs(Filesystem fs) {
        Filesystems.add(fs);
    }

    CPUDescription& cpu() { return CPU; }

    SinglyLinkedList<SystemDevice>& devices() { return Devices; }
    SystemDevice* device(u64 major, u64 minor) {
        Devices.for_each([major, minor](auto* it) {
            SystemDevice& dev = it->value();
            if (dev.major() == major && dev.minor() == minor)
                return dev;
        });
        return nullptr;
    }

    void print() {
        CPU.print_debug();
        Devices.for_each([](auto* it){
            SystemDevice& dev = it->value();
            UART::out("System Device ");
            UART::out(dev.major());
            UART::out(".");
            UART::out(dev.minor());
            UART::out(":\r\n  Data1: 0x");
            UART::out(to_hexstring(dev.data1()));
            UART::out("\r\n  Data2: 0x");
            UART::out(to_hexstring(dev.data2()));
            UART::out("\r\n  Data3: 0x");
            UART::out(to_hexstring(dev.data3()));
            UART::out("\r\n  Data4: 0x");
            UART::out(to_hexstring(dev.data4()));
            UART::out("\r\n");
        });
        UART::out("\r\n");
        Filesystems.for_each([](auto* it){
            Filesystem& fs = it->value();
            UART::out("Filesystem: ");
            UART::out(fstype_to_string(fs.type()));
            UART::out("\r\n  Filesystem Driver Address: 0x");
            UART::out(to_hexstring(fs.filesystem_driver()));
            UART::out("\r\n  Storage Device Driver Address: 0x");
            UART::out(to_hexstring(fs.storage_device_driver()));
            UART::out("\r\n");
        });
        UART::out("\r\n");
    }

private:
    CPUDescription CPU;
    SinglyLinkedList<SystemDevice> Devices;
    SinglyLinkedList<Filesystem> Filesystems;
};

extern System* SYSTEM;

#endif /* LENSOR_OS_SYSTEM_H */
