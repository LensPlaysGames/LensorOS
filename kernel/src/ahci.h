#ifndef LENSOR_OS_AHCI_H
#define LENSOR_OS_AHCI_H

#include <integers.h>
#include <system.h>

/// Size of AHCI Port buffer (how much the hardware reads/writes at a time)
/// 128mib = 134217700 bytes = 32768 pages = 0x8000
/// 1mib = 1048576 bytes = 256 pages = 0x100
#define MAX_READ_PAGES 0x100
#define MAX_READ_BYTES (MAX_READ_PAGES * 0x1000)

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ  0x08
#define ATA_CMD_READ_DMA_EX 0x25

#define HBA_PORT_DEVICE_PRESENT 0x3
#define HBA_PORT_IPM_ACTIVE     0x1
// Px == Port x where x is 0-31 inclusive.
#define HBA_PxCMD_CR   0x8000
#define HBA_PxCMD_FR   0x4000
#define HBA_PxCMD_FRE  0x10
#define HBA_PxCMD_ST   1
#define HBA_PxIS_TFES (1 << 30)

constexpr u32 HBA_PRDT_INTERRUPT_ON_COMPLETION = (1 << 31);

#define SATA_SIG_ATAPI 0xeb140101
#define SATA_SIG_ATA   0x00000101
#define SATA_SIG_SEMB  0xc33c0101
#define SATA_SIG_PM    0x96690101

/// AHCI (Advance Host Controller Interface) developed by Intel
///   Facilitates handling of Serial ATA devices.
///   An AHCI Controller is referred to as a Host Bus Adapter, or HBA.
///   The HBA's job (nutshell) is to allow access to SATA drives using
///     system memory and memory mapped registers, without using what
///     are called task files (a requirement for IDE, the alternative).
///   The HBA does this through the use of ports. A port is a pathway
///     for communication between a host (the computer) and a SATA device.
///   An HBA may support up to 32 ports which can attach different
///     SATA devices (disk drives, port multipliers, etc).
///   By sending commands through these ports, the devices can be manipulated
///     to do anything they are capable of (read data, write data, etc).
///
///   NOTE: All memory addresses used by the HBA must be physical.
///         This is done by mapping them 1:1 using the PTM.

namespace AHCI {
    /// Host Bus Adapter Port
    struct HBAPort {
        u32 CommandListBaseAddress;
        u32 CommandListBaseAddressUpper;
        u32 FISBaseAddress;
        u32 FISBaseAddressUpper;
        u32 InterruptStatus;
        u32 InterruptEnable;
        u32 CommandAndStatus;
        u32 Reserved0;
        u32 TaskFileData;
        u32 signature;
        u32 SataStatus;
        u32 SataControl;
        u32 SataError;
        u32 SataActive;
        u32 CommandIssue;
        u32 SataNotification;
        u32 FISSwitchControl;
        u32 Reserved1[11];
        u32 Vendor[4];

        void set_command_list_base(void* commandListBaseAddress) volatile {
            u64 commandListBase = reinterpret_cast<u64>(commandListBaseAddress);
            CommandListBaseAddress = static_cast<u32>(commandListBase);
            CommandListBaseAddressUpper = static_cast<u32>(commandListBase >> 32);
        }

        void set_frame_information_structure_base(void* fisBaseAddress) volatile {
            u64 fisBase = reinterpret_cast<u64>(fisBaseAddress);
            FISBaseAddress = static_cast<u32>(fisBase);
            FISBaseAddressUpper = static_cast<u32>(fisBase >> 32);
        }

        u64 command_list_base() volatile {
            return static_cast<u64>(CommandListBaseAddress)
                + (static_cast<u64>(CommandListBaseAddressUpper) << 32);
        }
        
        u64 frame_information_structure_base() volatile {
            return static_cast<u64>(FISBaseAddress)
                + (static_cast<u64>(FISBaseAddressUpper) << 32);
        }
    };

    /// Host Bus Adapter Memory Registers
    ///   The layout of the memory registers
    ///     accessable through the Host Bus Adapter.
    struct HBAMemory {
        u32 HostCapability;
        u32 GlobalHostControl;
        u32 InterruptStatus;
        u32 PortsImplemented;
        u32 Version;
        u32 cccControl;
        u32 cccPorts;
        u32 EnclosureManagementLocation;
        u32 EnclosureManagementControl;
        u32 HostCapabilitiesExtended;
        u32 BIOSHandoffControlStatus;
        u8 Reserved0[0x74];
        u8 Vendor[0x60];
        HBAPort Ports[1];
    };

    /// Host Bus Adapter Command Header
    ///   The beginning of a Host Bus Adapter command is structured as shown.
    // FIXME: Get rid of bitfields!
    struct HBACommandHeader {
        u8 CommandFISLength :5;
        u8 ATAPI            :1;
        u8 Write            :1;
        u8 Prefetchable     :1;
        u8 Reset            :1;
        u8 BIST             :1;
        u8 ClearBusy        :1;
        u8 Reserved0        :1;
        u8 PortMultiplier   :4;
        u16 PRDTLength;
        u32 PRDBCount;
        u32 CommandTableBaseAddress;
        u32 CommandTableBaseAddressUpper;
        u32 Reserved1[4];

        void set_command_table_base(u64 commandTableBase) {
            CommandTableBaseAddress = static_cast<u32>(commandTableBase);
            CommandTableBaseAddressUpper = static_cast<u32>(commandTableBase >> 32);
        }

        void set_command_table_base(void* commandTableBaseAddress) {
            set_command_table_base(reinterpret_cast<u64>(commandTableBaseAddress));
        }

        u64 command_table_base() {
            return static_cast<u64>(CommandTableBaseAddress)
                + (static_cast<u64>(CommandTableBaseAddressUpper) << 32);
        }
    };

    /// Host Bus Adapter Physical Region Descriptor Table Entry
    ///   Specifies data payload address in memory as well as size.
    struct HBA_PRDTEntry {
        u32 DataBaseAddress;
        u32 DataBaseAddressUpper;
        u32 Reserved0;
        /* 0b00000000000000000000000000000000
         *             ======================  Byte Count
         *    =========                        Reserved1
         *   =                                 Int. on Completion
         */
        u32 Information;


        void set_data_base(u64 newDataBaseAddress) {
            DataBaseAddress = static_cast<u32>(newDataBaseAddress);
            DataBaseAddressUpper = static_cast<u32>(newDataBaseAddress >> 32);
        }

        void set_byte_count(u32 newByteCount) {
            // Clear byte count bits to zero.
            Information &= ~0b1111111111111111111111;
            // Set byte count bits from new byte count.
            Information |= newByteCount & 0b1111111111111111111111;
        }

        void set_interrupt_on_completion(bool value) {
            // Clear the int. on completion bit.
            Information &= ~(HBA_PRDT_INTERRUPT_ON_COMPLETION);
            // Set the int. on completion bit if passed value is true.
            if (value)
                Information |= HBA_PRDT_INTERRUPT_ON_COMPLETION;
        }

        u64 data_base() {
            return static_cast<u64>(DataBaseAddress)
                + (static_cast<u64>(DataBaseAddressUpper) << 32);
        }

        u32 byte_count() {
            return Information & (0b1111111111111111111111);
        }

        bool interrupt_on_completion() {
            return Information & HBA_PRDT_INTERRUPT_ON_COMPLETION;
        }
    };

    struct HBACommandTable {
        u8 CommandFIS[64];
        u8 ATAPICommand[16];
        u8 Reserved[48];
        // FIXME: Is this supposed to be a variable length array?
        HBA_PRDTEntry PRDTEntry[];
    };

    /// Frame Information Structure Type
    enum FIS_TYPE {
        /// Used by the host to send command or control to a device.
        REG_H2D   = 0x27,
        /// Used by the device to notify the host
        ///   that some ATA register has changed.
        REG_D2H   = 0x34,
        DMA_ACT   = 0x39,
        DMA_SETUP = 0x41,
        /// Used by both the host and device to send data payload.
        DATA      = 0x46,
        BIST      = 0x58,
        /// Used by the device to notify the host that it's about to send
        ///   (or ready to recieve) a PIO data payload.
        PIO_SETUP = 0x5f,
        DEV_BITS  = 0xa1,
    };
    
    /// Frame Information Structure Reegister Host to Device
    struct FIS_REG_H2D {
        // FIXME: Get rid of bitfields!
        u8 Type;
        u8 PortMultiplier:4;
        u8 Reserved0:3;
        u8 CommandControl:1;
        u8 Command;
        u8 FeatureLow;
        u8 LBA0;
        u8 LBA1;
        u8 LBA2;
        u8 DeviceRegister;
        u8 LBA3;
        u8 LBA4;
        u8 LBA5;
        u8 FeatureHigh;
        u8 CountLow;
        u8 CountHigh;
        u8 ISOCommandCompletion;
        u8 Control;
        u8 Reserved1[4];

        void set_logical_block_addresses(u64 sector) {
            u32 sectorLow = static_cast<u32>(sector);
            u32 sectorHigh = static_cast<u32>(sector >> 32);
            LBA0 = static_cast<u8>(sectorLow);
            LBA1 = static_cast<u8>(sectorLow >> 8);
            LBA2 = static_cast<u8>(sectorLow >> 16);
            LBA3 = static_cast<u8>(sectorHigh);
            LBA4 = static_cast<u8>(sectorHigh >> 8);
            LBA5 = static_cast<u8>(sectorHigh >> 16);
        }

        void set_feature(u16 newFeature) {
            FeatureLow = static_cast<u8>(newFeature);
            FeatureHigh = static_cast<u8>(newFeature >> 8);
        }

        void set_count(u16 newCount) {
            CountLow = static_cast<u8>(newCount);
            CountHigh = static_cast<u8>(newCount >> 8);
        }

        u16 feature() {
            return static_cast<u16>(FeatureLow)
                + (static_cast<u16>(FeatureHigh) << 8);
        }

        u16 count() {
            return static_cast<u16>(CountLow)
                + (static_cast<u16>(CountHigh) << 8);
        }
    };

    /// Port Type
    enum PortType {
        None = 0,
        SATA = 1,
        SEMB = 2,
        PM = 3,
        SATAPI = 4
    };

    const char* port_type_string(PortType);
    PortType get_port_type(HBAPort* port);

    class PortController final : public StorageDeviceDriver {
    public:
        PortController(PortType type, u64 portNumber, HBAPort* portAddress);

        /// Convert bytes to sectors, then read into and copy from intermediate
        /// `Buffer` to given `buffer` until all data is read and copied.
        void read(u64 byteOffset, u64 byteCount, u8* buffer) final;
        void write(u64 byteOffset, u64 byteCount, u8* buffer) final;

        u64 port_number() { return PortNumber; }

    private:
        PortType Type { PortType::None };
        u64 PortNumber { 99 };
        volatile HBAPort* Port { nullptr };
        u8* Buffer { nullptr };
        const u64 BYTES_PER_SECTOR = 512;
        const u64 PORT_BUFFER_PAGES = 0x100;
        const u64 PORT_BUFFER_BYTES = PORT_BUFFER_PAGES * 0x1000;

        /// Populate `Buffer` with `sectors` amount of data starting at `sector`.
        bool read_low_level(u64 sector, u64 sectors);

        void start_commands();
        void stop_commands();
    };
}

#endif /* LENSOR_OS_AHCI_H */
