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

	const char* get_subclass_name(uint8_t _class, uint8_t subclass) {
	    switch (_class) {
		case 0x00:
			switch (subclass) {
			case 0x0:
				return "Non-VGA-Compatible Unclassified Device";
			case 0x1:
				return "VGA-Compatible Unclassified Device";
			default:
				return to_hexstring(subclass);
			}
		case 0x01:
			// Mass Storage Controller
			switch (subclass) {
			case 0x0:
				return "SCSI Bus Controller";
			case 0x1:
				return "IDE Controller";
			case 0x2:
				return "Floppy Disk Controller";
			case 0x3:
				return "IPI Bus Controller";
			case 0x4:
				return "RAID Controller";
			case 0x5:
				return "ADA Controller";
			case 0x6:
				return "Serial ATA Controller";
			case 0x7:
				return "Serial Attached SCSI Controller";
			case 0x8:
				return "Non-Volatile Memory Controller";
			case 0x80:
				return "Other";
			default:
				return to_hexstring(subclass);
			}
		case 0x02:
			// Network Controller
			switch (subclass) {
			case 0x0:
				return "Ethernet Controller";
			case 0x1:
				return "Token Ring Controller";
			case 0x2:
				return "FDDI Controller";
			case 0x3:
				return "ATM Controller";
			case 0x4:
				return "ISDN Controller";
			case 0x5:
				return "WorldFip Controller";
			case 0x6:
				return "PICMG 2.14 Multi Computing Controller";
			case 0x7:
				return "Infiniband Controller";
			case 0x8:
				return "Fabric Controller";
			case 0x80:
				return "Other";
			default:
				return to_hexstring(subclass);
			}
		case 0x03:
			// Display Controller
			switch (subclass) {
			case 0x0:
				return "VGA Compatible Controller";
			case 0x1:
				return "XGA Controller";
			case 0x2:
				return "3D Controller (Not VGA-Compatible) Controller";
			case 0x80:
				return "Other";
			default:
				return to_hexstring(subclass);
			}
		case 0x04:
			// Multimedia Controller
			switch (subclass) {
			case 0x0:
				return "Multimedia Video Controller";
			case 0x1:
				return "Multimedia Audio Controller";
			case 0x2:
				return "Computer Telephony Device";
			case 0x3:
				return "Audio Device";
			case 0x80:
				return "Other";
			default:
				return to_hexstring(subclass);
			}
		case 0x05:
			// Memory Controller
			switch (subclass) {
			case 0x0:
				return "RAM Controller";
			case 0x1:
				return "Flash Controller";
			case 0x80:
				return "Other";
			default:
				return to_hexstring(subclass);
			}
		case 0x06:
			// Bridge
			switch (subclass) {
			case 0x0:
				return "Host Bridge";
			case 0x1:
				return "ISA Bridge";
			case 0x2:
				return "EISA Bridge";
			case 0x3:
				return "MCA Bridge";
			case 0x4:
				return "PCI-to-PCI Bridge";
			case 0x5:
				return "PCMCIA Bridge";
			case 0x6:
				return "NuBus Bridge";
			case 0x7:
				return "CardBus Bridge";
			case 0x8:
				return "RACEway Bridge";
			case 0x9:
				return "PCI-to-PCI Bridge";
			case 0xa:
				return "InfiniBand-to-PCI Host Bridge";
			case 0x80:
				return "Other";
			default:
				return to_hexstring(subclass);
			}
		default:
			return to_hexstring(subclass);
		}
	}

	const char* get_prog_if_name(uint8_t _class, uint8_t subclass, uint8_t progIF) {
		switch (_class) {
		case 0x01:
			// Mass Storage Controller
			switch (subclass) {
			case 0x1:
				// IDE Controller
				switch(progIF) {
				case 0x0:
					return "ISA compatibility mode-only controller";
				case 0x5:
					return "PCI native mode-only controller";
				case 0xa:
					return "ISA compatibility mode controller, supports both channels switched to PCI native mode";
				case 0xf:
					return "PCI native mode controller, supports both channels switched to ISA compatibility mode";
				case 0x80:
					return "ISA compatibility mode-only controller, supports bus mastering";
				case 0x85:
					return "PCI native mode-only controller, supports bus mastering";
				case 0x8a:
					return "ISA compatibility mode controller, supports both channels switched to PCI native mode, supports bus mastering";
				case 0x8f:
					return "PCI native mode controller, supports both channels switched to ISA compatibility mode, supports bus mastering";
				default:
					return to_hexstring(progIF);
				}
			case 0x5:
				// ATA Controller
				switch(progIF) {
				case 0x20:
					return "Single DMA";
				case 0x30:
					return "Chained DMA";
				default:
					return to_hexstring(progIF);
				}
			case 0x6:
				// Serial ATA Controller
				switch(progIF) {
				case 0x0:
					return "Vendor Specific Interface";
				case 0x1:
					return "AHCI 1.0";
				case 0x2:
					return "Serial Storage Bus";
				default:
					return to_hexstring(progIF);
				}
			case 0x7:
				// Serial Attached SCSI Controller
				switch(progIF) {
				case 0x0:
					return "SAS";
				case 0x1:
					return "Serial Storage Bus";
				default:
					return to_hexstring(progIF);
				}
			case 0x8:
				// Non-Volatile Memory Controller
				switch(progIF) {
				case 0x1:
					return "NVMHCI";
				case 0x2:
					return "NVM Express";
				default:
					return to_hexstring(progIF);
				}
			default:
				return to_hexstring(progIF);
			}
		case 0x03:
			// Display Controller
			if (subclass == 0x0) {
				// VGA Compatible Controller
				switch (progIF) {
				case 0x0:
					return "VGA Controller";
				case 0x1:
					return "8514-Compatible Controller";
				default:
					return to_hexstring(progIF);
				}
			}
		case 0x06:
			// Bridge
			switch (subclass) {
			case 0x4: 
				// PCI-to-PCI Bridge
				switch (progIF) {
				case 0x0:
					return "Normal Decode";
				case 0x1:
					return "Subtractive Decode";
				default:
					return to_hexstring(progIF);
				}
			case 0x8: 
				// RACEway Bridge
				switch (progIF) {
				case 0x0:
					return "Transparent Mode";
				case 0x1:
					return "Endpoint Mode";
				default:
					return to_hexstring(progIF);
				}
			case 0x9: 
				// PCI-to-PCI Bridge
				switch (progIF) {
				case 0x40:
					return "Semi-Transparent, Primary bus towards host CPU";
				case 0x80:
					return "Semi-Transparent, Secondary bus towards host CPU";
				default:
					return to_hexstring(progIF);
				}
			}
		default:
			return to_hexstring(progIF);
		}
	}
}
