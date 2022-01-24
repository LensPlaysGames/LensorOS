#include "ahci.h"

namespace AHCI {
#define HBA_PORT_DEVICE_PRESENT 0x3
#define HBA_PORT_IPM_ACTIVE     0x1

#define SATA_SIG_ATAPI 0xeb140101
#define SATA_SIG_ATA   0x00000101
#define SATA_SIG_SEMB  0xc33c0101
#define SATA_SIG_PM    0x96690101

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

		if (port->signature == SATA_SIG_ATAPI) {
			return PortType::SATAPI;
		}
		if (port->signature == SATA_SIG_ATA) {
			return PortType::SATA;
		}
		if (port->signature == SATA_SIG_SEMB) {
			return PortType::SEMB;
		}
		if (port->signature == SATA_SIG_PM) {
			return PortType::PM;
		}
		return PortType::None;
	}

	void AHCIDriver::probe_ports() {
		uint32_t ports = ABAR->portsImplemented;
		for (uint64_t i = 0; i < 32; ++i) {
			if (ports & (1 << i)) {
				PortType type = get_port_type(&ABAR->ports[i]);
				gRend.putstr(to_string(i));
				gRend.putstr(": ");
				if (type == PortType::SATA) {
					gRend.putstr("SATA Drive");
				}
				else if (type == PortType::SATAPI) {
					gRend.putstr("SATAPI Drive");
				}
				gRend.crlf();
			}
		}
	}

	AHCIDriver::AHCIDriver(PCI::PCIDeviceHeader* pciBaseAddress) {
		PCIBaseAddress = pciBaseAddress;
		
		gRend.crlf();
		gRend.putstr("Assigning ABAR.");
		gRend.crlf();
		ABAR = (HBAMemory*)((uint64_t)(((PCI::PCIHeader0*)PCIBaseAddress)->BAR5));

		gRend.putstr("Mapping ABAR memory.");
		gRend.crlf();
		gPTM.map_memory(ABAR, ABAR);
		
		gRend.putstr("Probing ports.");
		gRend.crlf();
		probe_ports();
		gRend.putstr("Ports probed.");
		gRend.crlf();
	}
	
	AHCIDriver::~AHCIDriver() {
		
	}
}
