#ifndef LENSOR_OS_AHCI_H
#define LENSOR_OS_AHCI_H

#include "integers.h"
#include "spinlock.h"

namespace PCI {
    class PCIDeviceHeader;
    class PCIHeader0;
}

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
/// Max readable file size
/// 128mib = 134217700 bytes = 32768 pages = 0x8000
/// 1mib = 1048576 bytes = 256 pages = 0x100
#define MAX_READ_PAGES 0x100
    
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ  0x08
#define ATA_CMD_READ_DMA_EX 0x25

#define HBA_PxIS_TFES (1 << 30)
    
    /// Host Bus Adapter Port
    struct HBAPort{
        u32 commandListBase;
        u32 commandListBaseUpper;
        u32 fisBaseAddress;
        u32 fisBaseAddressUpper;
        u32 interruptStatus;
        u32 interruptEnable;
        u32 cmdSts;
        u32 rsv0;
        u32 taskFileData;
        u32 signature;
        u32 sataStatus;
        u32 sataControl;
        u32 sataError;
        u32 sataActive;
        u32 commandIssue;
        u32 sataNotification;
        u32 fisSwitchControl;
        u32 rsv1[11];
        u32 vendor[4];
    };

    /// Host Bus Adapter Memory Registers
    ///   The layout of the memory registers
    ///     accessable through the Host Bus Adapter.
    struct HBAMemory{
        u32 hostCapability;
        u32 globalHostControl;
        u32 interruptStatus;
        u32 portsImplemented;
        u32 version;
        u32 cccControl;
        u32 cccPorts;
        u32 enclosureManagementLocation;
        u32 enclosureManagementControl;
        u32 hostCapabilitiesExtended;
        u32 biosHandoffCtrlSts;
        u8 rsv0[0x74];
        u8 vendor[0x60];
        HBAPort ports[1];
    };

    /// Host Bus Adapter Command Header
    ///   The beginning of a Host Bus Adapter command is structured as shown.
    struct HBACommandHeader {
        u8 commandFISLength :5;
        u8 atapi            :1;
        u8 write            :1;
        u8 prefetchable     :1;
        u8 reset            :1;
        u8 bist             :1;
        u8 clearBusy        :1;
        u8 rsv0             :1;
        u8 portMultiplier   :4;
        u16 prdtLength;
        u32 prdbCount;
        u32 commandTableBaseAddress;
        u32 commandTableBaseAddressUpper;
        u32 rsv1[4];
    };

    /// Host Bus Adapter Physical Region Descriptor Table Entry
    ///   Specifies data payload address in memory as well as size.
    struct HBA_PRDTEntry {
        u32 dataBaseAddress;
        u32 dataBaseAddressUpper;
        u32 rsv0;
        u32 byteCount             :22;
        u32 rsv1                  :9;
        u32 interruptOnCompletion :1;
    };

    struct HBACommandTable {
        u8 commandFIS[64];
        u8 atapiCommand[16];
        u8 rsv[48];
        HBA_PRDTEntry prdtEntry[];
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
        u8 type;
        u8 portMultiplier:4;
        u8 rsv0:3;
        u8 commandControl:1;
        u8 command;
        u8 featureLow;
        u8 lba0;
        u8 lba1;
        u8 lba2;
        u8 deviceRegister;
        u8 lba3;
        u8 lba4;
        u8 lba5;
        u8 featureHigh;
        u8 countLow;
        u8 countHigh;
        u8 isoCommandCompletion;
        u8 control;
        u8 rsv1[4];
    };

    /// Port Type
    enum PortType {
        None = 0,
        SATA = 1,
        SEMB = 2,
        PM = 3,
        SATAPI = 4
    };
    
    class Port {
    public:
        Spinlock lock;
        HBAPort* hbaPort;
        PortType type;
        u8* buffer;
        u8  number;
        void Configure();
        void StartCMD();
        void StopCMD();
        bool read_low_level(u64 sector, u16 numSectors);
        bool read(u64 sector, u16 numSectors, void* buffer, u64 numBytesToCopy);
    };

    /// Advance Host Controller Interface Driver
    ///   This driver is instantiated for each SATA controller found on
    ///     the PCI bus, and will parse all the ports that are active and
    ///     valid for later use.
    // TODO: Store created driver in some sort of device tree
    class AHCIDriver {
    public:
        Port* Ports[32];
        u8 numPorts;

        AHCIDriver(PCI::PCIDeviceHeader* pciBaseAddress);
        ~AHCIDriver();

        void probe_ports();

        bool read(u64 sector, u64 numSectors, u8 portNumber, void* buffer, u64 numBytes) {
            if (portNumber >= numPorts)
                return false;
            
            return Ports[portNumber]->read(sector, numSectors, buffer, numBytes);
        }
        
    private:
        /// Address of PCI device header (expected SATA Controller, AHCI 1.0).
        PCI::PCIDeviceHeader* PCIBaseAddress;
        /// AHCI Base Memory Register
        HBAMemory* ABAR;        
    };

    /// Store list of pointers to drivers that are created for later use.
    // Honestly I'm not sure if this is needed, but I'm keeping it here until I can prove that I don't.
    extern AHCIDriver** Drivers;
    extern u16 NumDrivers;
}

#endif
