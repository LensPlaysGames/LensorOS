#include "ahci.h"
#include "filesystems/fs_fat.h"

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

	AHCIDriver* gAHCI;

	PortType get_port_type(HBAPort* port) {
		uint32_t sataStatus = port->sataStatus;
		uint8_t interfacePowerManagement = (sataStatus >> 8) & 0b111;
		uint8_t deviceDetection = sataStatus & 0b111;

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
		uint32_t ports = ABAR->portsImplemented;
		for (uint64_t i = 0; i < 32; ++i) {
			if (ports & (1 << i)) {
				PortType type = get_port_type(&ABAR->ports[i]);
				if (type == PortType::SATA
					|| type == PortType::SATAPI)
				{
					Ports[numPorts].hbaPort = &ABAR->ports[i];
					Ports[numPorts].type = type;
					Ports[numPorts].number = numPorts;
					numPorts++;
				}
			}
		}
	}

	void Port::Configure() {
		StopCMD();
		// Command Base
		void* base = gAlloc.request_page();
	    hbaPort->commandListBase = (uint32_t)(uint64_t)base;
	    hbaPort->commandListBaseUpper = (uint32_t)((uint64_t)base >> 32);
		memset(base, 0, 1024);
		// FIS Base
		void* fisBase = gAlloc.request_page();
	    hbaPort->fisBaseAddress = (uint32_t)(uint64_t)fisBase;
	    hbaPort->fisBaseAddressUpper = (uint32_t)((uint64_t)fisBase >> 32);
		memset(fisBase, 0, 256);

		HBACommandHeader* cmdHdr = (HBACommandHeader*)((uint64_t)hbaPort->commandListBase + ((uint64_t)hbaPort->commandListBaseUpper << 32));
		for (uint64_t i = 0; i < 32; ++i) {
			cmdHdr[i].prdtLength = 8;
			void* cmdTableAddress = gAlloc.request_page();
			uint64_t address = (uint64_t)cmdTableAddress + (i << 8);
			cmdHdr[i].commandTableBaseAddress = (uint32_t)address;
			cmdHdr[i].commandTableBaseAddressUpper = (uint32_t)((uint64_t)address >> 32);
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

	bool Port::Read(uint64_t sector, uint16_t numSectors, void* buffer) {
		const uint64_t maxSpin = 1000000;
		uint64_t spin = 0;
		while ((hbaPort->taskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ))
			   && spin < maxSpin)
		{
			spin++;
		}
		if (spin == maxSpin) {
			return false;
		}

		uint32_t sectorL = (uint32_t)sector;
		uint32_t sectorH = (uint32_t)(sector >> 32);

		hbaPort->interruptStatus = (uint32_t)-1;

		HBACommandHeader* cmdHdr = (HBACommandHeader*)(uint64_t)hbaPort->commandListBase;
		cmdHdr->commandFISLength = sizeof(FIS_REG_H2D)/sizeof(uint32_t);
		cmdHdr->write = 0;
		cmdHdr->prdtLength = 1;
		HBACommandTable* cmdTable = (HBACommandTable*)(uint64_t)cmdHdr->commandTableBaseAddress;
		memset(cmdTable, 0, sizeof(HBACommandTable) + ((cmdHdr->prdtLength-1) * sizeof(HBA_PRDTEntry)));
		cmdTable->prdtEntry[0].dataBaseAddress = (uint32_t)(uint64_t)buffer;
		cmdTable->prdtEntry[0].dataBaseAddressUpper = (uint32_t)((uint64_t)buffer >> 32);
		cmdTable->prdtEntry[0].byteCount = (numSectors << 9) - 1;
		cmdTable->prdtEntry[0].interruptOnCompletion = 1;
		FIS_REG_H2D* cmdFIS = (FIS_REG_H2D*)(&cmdTable->commandFIS);
		cmdFIS->type = FIS_TYPE::REG_H2D;
		// take control of command
		cmdFIS->commandControl = 1;
		cmdFIS->command = ATA_CMD_READ_DMA_EX;
		// assign lba's
		cmdFIS->lba0 = (uint8_t)(sectorL);
		cmdFIS->lba1 = (uint8_t)(sectorL >> 8);
		cmdFIS->lba2 = (uint8_t)(sectorL >> 16);
		cmdFIS->lba3 = (uint8_t)(sectorH);
		cmdFIS->lba4 = (uint8_t)(sectorH >> 8);
		cmdFIS->lba5 = (uint8_t)(sectorH >> 16);
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
		PCIBaseAddress = pciBaseAddress;
		
	    ABAR = (HBAMemory*)(uint64_t)(((PCI::PCIHeader0*)PCIBaseAddress)->BAR5);
		// Map ABAR into memory.
		gPTM.map_memory(ABAR, ABAR);
		
		gRend.putstr("Probing AHCI 1.0 Controller at ");
		gRend.putstr(to_hexstring((uint64_t)PCIBaseAddress));
		gRend.crlf();
		gRend.swap();
		
		// Probe ABAR for port info.
		probe_ports();
		
		gRend.putstr("Found ");
		gRend.putstr(to_string((uint64_t)numPorts));
		gRend.putstr(" open and active ports");
		gRend.crlf();
		gRend.putstr("Max read/write: ");
		gRend.putstr(to_string((uint64_t)MAX_READ_PAGES * 4));
		gRend.putstr("kib");
		gRend.crlf();
		gRend.swap();

		FatFS::FATDriver FAT;
		
		for(uint32_t i = 0; i < numPorts; ++i) {
			Ports[i].Configure();
			Ports[i].buffer = (uint8_t*)gAlloc.request_pages(MAX_READ_PAGES);
			if (Ports[i].buffer != nullptr) {
				memset((void*)Ports[i].buffer, 0, MAX_READ_PAGES * 0x1000);

				if (FAT.is_device_FAT(&Ports[i])) {
					gRend.putstr("Device is FAT formatted.");
					gRend.crlf();
				}
				
				gRend.putstr("Port ");
				gRend.putstr(to_string((uint64_t)i));
				gRend.putstr(" successfully configured");
				gRend.crlf();
				gRend.swap();
			}
			else {
				gRend.putstr("Port ");
				gRend.putstr(to_string((uint64_t)i));
				gRend.putstr(" could not be configured");
				gRend.crlf();
				gRend.swap();
			}
			// READ ALL DISKS' CONTENTS TO SCREEN
			// (requires '#include "../basic_renderer.h"')
			// if (Ports[i].Read(0, 4, Ports[i].buffer)) {
			// 	for (int t = 0; t < 1024; ++t) {
			// 		gRend.putchar(Ports[i].buffer[t]);
			// 	}
			// 	gRend.crlf();
			// 	gRend.swap();
			// }
		}
	}
	
	AHCIDriver::~AHCIDriver() {
		gRend.putstr("Deconstructing AHCI Driver");
		gRend.crlf();
		gRend.swap();
		for(uint32_t i = 0; i < numPorts; ++i) {
			gAlloc.free_pages((void*)Ports[i].buffer, MAX_READ_PAGES);
		}
	}
}
