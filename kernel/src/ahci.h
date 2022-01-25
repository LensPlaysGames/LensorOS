#ifndef LENSOR_OS_AHCI_H
#define LENSOR_OS_AHCI_H

#include <stdint.h>
#include "paging/page_table_manager.h"
#include "pci.h"

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
        uint32_t commandListBase;
        uint32_t commandListBaseUpper;
        uint32_t fisBaseAddress;
        uint32_t fisBaseAddressUpper;
        uint32_t interruptStatus;
        uint32_t interruptEnable;
        uint32_t cmdSts;
        uint32_t rsv0;
        uint32_t taskFileData;
        uint32_t signature;
        uint32_t sataStatus;
        uint32_t sataControl;
        uint32_t sataError;
        uint32_t sataActive;
        uint32_t commandIssue;
        uint32_t sataNotification;
        uint32_t fisSwitchControl;
        uint32_t rsv1[11];
        uint32_t vendor[4];
    };

	/// Host Bus Adapter Memory Registers
	///   The layout of the memory registers
	///     accessable through the Host Bus Adapter.
	struct HBAMemory{
        uint32_t hostCapability;
        uint32_t globalHostControl;
        uint32_t interruptStatus;
        uint32_t portsImplemented;
        uint32_t version;
        uint32_t cccControl;
        uint32_t cccPorts;
        uint32_t enclosureManagementLocation;
        uint32_t enclosureManagementControl;
        uint32_t hostCapabilitiesExtended;
        uint32_t biosHandoffCtrlSts;
        uint8_t rsv0[0x74];
        uint8_t vendor[0x60];
        HBAPort ports[1];
    };

	/// Host Bus Adapter Command Header
	///   The beginning of a Host Bus Adapter command is structured as shown.
	struct HBACommandHeader {
		uint8_t commandFISLength :5;
		uint8_t atapi            :1;
		uint8_t write            :1;
		uint8_t prefetchable     :1;
		uint8_t reset            :1;
		uint8_t bist             :1;
		uint8_t clearBusy        :1;
		uint8_t rsv0             :1;
		uint8_t portMultiplier   :4;
		uint16_t prdtLength;
		uint32_t prdbCount;
		uint32_t commandTableBaseAddress;
		uint32_t commandTableBaseAddressUpper;
		uint32_t rsv1[4];
	};

	/// Host Bus Adapter Physical Region Descriptor Table Entry
	///   Specifies data payload address in memory as well as size.
	struct HBA_PRDTEntry {
		uint32_t dataBaseAddress;
		uint32_t dataBaseAddressUpper;
		uint32_t rsv0;
		uint32_t byteCount             :22;
		uint32_t rsv1                  :9;
		uint32_t interruptOnCompletion :1;
	};

	struct HBACommandTable {
		uint8_t commandFIS[64];
		uint8_t atapiCommand[16];
		uint8_t rsv[48];
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
		uint8_t type;
		uint8_t portMultiplier:4;
		uint8_t rsv0:3;
		uint8_t commandControl:1;
		uint8_t command;
		uint8_t featureLow;
		uint8_t lba0;
		uint8_t lba1;
		uint8_t lba2;
		uint8_t deviceRegister;
		uint8_t lba3;
		uint8_t lba4;
		uint8_t lba5;
		uint8_t featureHigh;
		uint8_t countLow;
		uint8_t countHigh;
		uint8_t isoCommandCompletion;
		uint8_t control;
		uint8_t rsv1[4];
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
		HBAPort* hbaPort;
		PortType type;
		uint8_t* buffer;
		uint8_t  number;
		void Configure();
		void StartCMD();
		void StopCMD();
		bool Read(uint64_t sector, uint16_t numSectors, void* buffer);
	};

	/// Advance Host Controller Interface Driver
	///   This driver is instantiated for each SATA controller found on
	///     the PCI bus, and will parse all the ports that are active and
	///     valid for later use.
	class AHCIDriver {
	public:
		/// Address of PCI device header (expected SATA Controller, AHCI 1.0).
		PCI::PCIDeviceHeader* PCIBaseAddress;
		/// AHCI Base Memory Register
		HBAMemory* ABAR;

		AHCIDriver(PCI::PCIDeviceHeader* pciBaseAddress);
		~AHCIDriver();

		// TODO: Move port memory allocation to dynamic (reduce memory usage).
		Port Ports[32];
		uint8_t numPorts;
		void probe_ports();
	};
}

#endif
