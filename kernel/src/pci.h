#ifndef LENSOR_OS_PCI_H
#define LENSOR_OS_PCI_H

#include "integers.h"
#include "acpi.h"
#include "paging/page_table_manager.h"
#include "heap.h"
#include "basic_renderer.h"
#include "cstr.h"

namespace PCI {
	struct PCIDeviceHeader {
		u16 VendorID;
		u16 DeviceID;
		u16 Command;
		u16 Status;
		u8 RevisionID;
		u8 ProgIF;
		u8 Subclass;
		u8 Class;
		u8 CacheLineSize;
		u8 LatencyTimer;
		u8 HeaderType;
		u8 BIST;
	};

	struct PCIHeader0 {
		PCIDeviceHeader Header;
		u32 BAR0;
		u32 BAR1;
		u32 BAR2;
		u32 BAR3;
		u32 BAR4;
		u32 BAR5;
		u32 CardbusCISPtr;
		u16 SubsystemVendorID;
		u16 SubsystemID;
		u32 ExpansionROMBaseAddress;
		u8 CapabilitiesPtr;
		u8 rsv0;
		u16 rsv1;
		u32 rsv2;
		u8 InterruptLine;
		u8 InterruptPin;
		u8 MinGrant;
		u8 MaxLatency;
	};
	
	void enumerate_pci(ACPI::MCFGHeader* mcfg);

	extern const char* DeviceClasses[];
	const char* get_vendor_name(u16 vendorID);
	const char* get_device_name(u16 vendorID, u16 deviceID);
	const char* get_subclass_name(u8 classCode, u8 subclassCode);
	const char* get_prog_if_name(u8 _class, u8 subclass, u8 progIF);
}
#endif
