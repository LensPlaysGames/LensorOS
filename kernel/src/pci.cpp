#include "pci.h"

namespace PCI {
	void enumerate_function(uint64_t device_address, uint64_t function_number) {
		uint64_t offset = function_number << 12;
		uint64_t function_address = device_address + offset;
		gPTM.MapMemory((void*)function_address, (void*)function_address);
		PCIDeviceHeader* pciDevHdr = (PCIDeviceHeader*)function_address;
		if (pciDevHdr->DeviceID == 0x0000) { return; }
		if (pciDevHdr->DeviceID == 0xFFFF) { return; }

		// Print information about device
		gRend.crlf();
		gRend.putstr(get_vendor_name(pciDevHdr->VendorID));
		gRend.putstr(" / ");
		gRend.putstr(get_device_name(pciDevHdr->VendorID, pciDevHdr->DeviceID));
		gRend.putstr(" / ");
		gRend.putstr(DeviceClasses[pciDevHdr->Class]);
		gRend.putstr(" / ");
		gRend.putstr(to_hexstring(pciDevHdr->Subclass));
		gRend.putstr(" / ");
		gRend.putstr(to_hexstring(pciDevHdr->ProgIF));
	}
	
	void enumerate_device(uint64_t bus_address, uint64_t device_number) {
		uint64_t offset = device_number << 15;
		uint64_t device_address = bus_address + offset;
		gPTM.MapMemory((void*)device_address, (void*)device_address);
		PCIDeviceHeader* pciDevHdr = (PCIDeviceHeader*)device_address;
		if (pciDevHdr->DeviceID == 0x0000) { return; }
		if (pciDevHdr->DeviceID == 0xFFFF) { return; }
		for (uint64_t function = 0; function < 8; ++function) {
			enumerate_function(device_address, function);
		}
	}
	
	void enumerate_bus(uint64_t base_address, uint64_t bus_number) {
		uint64_t offset = bus_number << 20;
		uint64_t bus_address = base_address + offset;
		gPTM.MapMemory((void*)bus_address, (void*)bus_address);
		PCIDeviceHeader* pciDevHdr = (PCIDeviceHeader*)bus_address;
		if (pciDevHdr->DeviceID == 0x0000) { return; }
		if (pciDevHdr->DeviceID == 0xFFFF) { return; }
		for (uint64_t device = 0; device < 32; ++device) {
			enumerate_device(bus_address, device);
		}
	}
	
	void enumerate_pci(ACPI::MCFGHeader* mcfg) {
		int entries = ((mcfg->Header.Length) - sizeof(ACPI::MCFGHeader)) / sizeof(ACPI::DeviceConfig);
		for (int t = 0; t < entries; ++t) {
			ACPI::DeviceConfig* devCon = (ACPI::DeviceConfig*)((uint64_t)mcfg + sizeof(ACPI::MCFGHeader) + (sizeof(ACPI::DeviceConfig) * t));
			for (uint64_t bus = devCon->StartBus; bus < devCon->EndBus; bus++) {
				enumerate_bus(devCon->BaseAddress, bus);
			}
			gRend.swap();
		}
	}
}
