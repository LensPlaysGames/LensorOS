#ifndef LENSOR_OS_AHCI_H
#define LENSOR_OS_AHCI_H

#include "../pci.h"

namespace AHCI {
	class AHCIDriver {
	public:
		AHCIDriver(PCI::PCIDeviceHeader* pciBaseAddress);
		~AHCIDriver();
		PCI::PCIDeviceHeader* PCIBaseAddress;
	};
}

#endif
