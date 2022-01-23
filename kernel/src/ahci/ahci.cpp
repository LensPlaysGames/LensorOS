#include "ahci.h"

namespace AHCI {
	PCI::PCIDeviceHeader* PCIBaseAddress;

	AHCIDriver::AHCIDriver(PCI::PCIDeviceHeader* pciBaseAddress) {
		PCIBaseAddress = pciBaseAddress;
	}
	
	AHCIDriver::~AHCIDriver() {
		
	}
}
