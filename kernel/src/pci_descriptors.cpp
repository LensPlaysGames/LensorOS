#include <stdint.h>
#include "cstr.h"

namespace PCI {
    const char* DeviceClasses[] {
        "Unclassified",
        "Mass Storage Controller",
        "Network Controller",
        "Display Controller",
        "Multimedia Controller",
        "Memory Controller",
        "Bridge Device",
        "Simple Communication Controller",
        "Base System Peripheral",
        "Input Device Controller",
        "Docking Station", 
        "Processor",
        "Serial Bus Controller",
        "Wireless Controller",
        "Intelligent Controller",
        "Satellite Communication Controller",
        "Encryption Controller",
        "Signal Processing Controller",
        "Processing Accelerator",
        "Non Essential Instrumentation"
    };

	const char* get_vendor_name(uint16_t vendorID) {
		switch (vendorID) {
		case 0x8086:
			return "Intel Corp";
		case 0x1022:
			return "AMD";
		case 0x10DE:
			return "NVIDIA Corporation";
		case 0x1013:
			return "Cirrus Logic";
		default:
			return to_hexstring(vendorID);
		}
	}

	const char* get_device_name(uint16_t vendorID, uint16_t deviceID) {
		switch (vendorID) {
		case 0x8086:
			// Intel Corp
			switch (deviceID) {
			case 0x29c0:
				return "82G33/G31/P35/P31 Express DRAM Controller";
			default:
				return to_hexstring(deviceID);
			}
		case 0x1022:
			// AMD
		    switch (deviceID) {
			default:
				return to_hexstring(deviceID);
			}
		case 0x10DE:
			// NVIDIA Corporation
		    switch (deviceID) {
			default:
				return to_hexstring(deviceID);
			}
		case 0x1013:
			// Cirrus Logic
		    switch (deviceID) {
			default:
				return to_hexstring(deviceID);
			}
		default:
			return to_hexstring(deviceID);
		}
	}
}
