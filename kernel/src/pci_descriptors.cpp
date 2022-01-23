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
			case 0x2918:
				return "82801IB (ICH9) LPC Interface Controller";
			case 0x2922:
				return "82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode]";
			case 0x2930:
				return "82801I (ICH9 Family) SMBus Controller";
			case 0x293e:
				return "82801I (ICH9 Family) HD Audio Controller";
			case 0x29c0:
				return "82G33/G31/P35/P31 Express DRAM Controller";
			case 0xa202:
				return "Lewisburg SATA Controller [AHCI mode]";
			case 0xa206:
				return "Lewisburg SATA Controller [RAID mode]";
			case 0xa252:
				return "Lewisburg SSATA Controller [AHCI mode]";
			case 0xa256:
				return "Lewisburg SSATA Controller [RAID mode]";
			case 0xa282:
				return "200 Series PCH SATA controller [AHCI mode]";
			case 0xa286:
				return "200 Series PCH SATA controller [RAID mode]";
			case 0xa382:
				return "400 Series Chipset Family SATA AHCI Controller";
			default:
				return to_hexstring(deviceID);
			}
		case 0x1022:
			// AMD
		    switch (deviceID) {
			case 0x43b5:
				return "X370 Series Chipset SATA Controller";
			case 0x43b6:
				return "X399 Series Chipset SATA Controller";
			case 0x43b7:
				return "300 Series Chipset SATA Controller";
			case 0x43c8:
				return "400 Series Chipset SATA Controller";
			case 0x7800:
			case 0x7900:
				return "FCH SATA Controller [IDE mode]";
			case 0x7801:
			case 0x7804:
			case 0x7901:
			case 0x7904:
				return "FCH SATA Controller [AHCI mode]";
			case 0x7802:
			case 0x7803:
			case 0x7805:
			case 0x7902:
			case 0x7903:
				return "FCH SATA Controller [RAID mode]";
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
			case 0x0038:
				return "GD 7548";
			case 0x0040:
				return "GD 7555 Flat Panel GUI Accelerator";
			case 0x004c:
				return "GD 7556 Video/Graphics LCD/CRT Ctrlr";
			case 0x00a0:
				return "GD 5430/40 [Alpine]";
			case 0x00a2:
				return "GD 5432 [Alpine]";
			case 0x00a4:
				return "GD 5436-4 [Alpine]";
			case 0x00a8:
				return "GD 5436-8 [Alpine]";
			case 0x00ac:
				return "GD 5436 [Alpine]";
			case 0x00b0:
				return "GD 5440";
			case 0x00b8:
				return "GD 5446";
			case 0x00bc:
				return "GD 5480";
			case 0x00d0:
				return "GD 5462";
			default:
				return to_hexstring(deviceID);
			}
		default:
			return to_hexstring(deviceID);
		}
	}

	const char* get_subclass_name(uint8_t classCode, uint8_t subclassCode) {
		return "";
	}
}
