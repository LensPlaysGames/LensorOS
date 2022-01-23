#ifndef LENSOR_OS_PCI_H
#define LENSOR_OS_PCI_H

#include <stdint.h>
#include "acpi.h"
#include "paging/page_table_manager.h"
#include "basic_renderer.h"
#include "cstr.h"

namespace PCI {
	struct PCIDeviceHeader {
		uint16_t VendorID;
		uint16_t DeviceID;
		uint16_t Command;
		uint16_t Status;
		uint8_t RevisionID;
		uint8_t ProgIF;
		uint8_t Subclass;
		uint8_t Class;
		uint8_t CacheLineSize;
		uint8_t LatencyTimer;
		uint8_t HeaderType;
		uint8_t BIST;
	};
	
	void enumerate_pci(ACPI::MCFGHeader* mcfg);

	extern const char* DeviceClasses[];
	const char* get_vendor_name(uint16_t vendorID);
	const char* get_device_name(uint16_t vendorID, uint16_t deviceID);
	const char* get_subclass_name(uint8_t classCode, uint8_t subclassCode);
	const char* get_prog_if_name(uint8_t progIF);

#endif
