#include <pci.h>

#include <ahci.h>
#include <acpi.h>
#include <cstr.h>
#include <memory/heap.h>
#include <memory/virtual_memory_manager.h>
#include <system.h>
#include <uart.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_PCI

namespace PCI {
    void print_device_header(PCIDeviceHeader* pci) {
        if (pci == nullptr)
            return;

        UART::out("[PCI]: Device Header at 0x");
        UART::out(to_hexstring(pci));
        UART::out("\r\n"
                  "  Vendor ID: ");
        UART::out(to_string(pci->VendorID));
        UART::out("\r\n"
                  "  Device ID: ");
        UART::out(to_string(pci->DeviceID));
        UART::out("\r\n"
                  "  Command: ");
        UART::out(to_string(pci->Command));
        UART::out("\r\n"
                  "  Status: ");
        UART::out(to_string(pci->Status));
        UART::out("\r\n"
                  "  Revision ID: ");
        UART::out(to_string(pci->RevisionID));
        UART::out("\r\n"
                  "  ProgIF: ");
        UART::out(to_string(pci->ProgIF));
        UART::out("\r\n"
                  "  Subclass: ");
        UART::out(to_string(pci->Subclass));
        UART::out("\r\n"
                  "  Class: ");
        UART::out(to_string(pci->Class));
        UART::out("\r\n"
                  "  Cache Line Size: ");
        UART::out(to_string(pci->CacheLineSize));
        UART::out("\r\n"
                  "  Latency Timer: ");
        UART::out(to_string(pci->LatencyTimer));
        UART::out("\r\n"
                  "  Header Type: ");
        UART::out(to_string(pci->HeaderType));
        UART::out("\r\n"
                  "  BIST: ");
        UART::out(to_string(pci->BIST));
        UART::out("\r\n");
    }

    void enumerate_function(u64 deviceAddress, u64 functionNumber) {
        u64 offset = functionNumber << 12;
        u64 function_address = deviceAddress + offset;
        Memory::map((void*)function_address, (void*)function_address);
        PCIDeviceHeader* pciDevHdr = reinterpret_cast<PCIDeviceHeader*>(function_address);
        if (pciDevHdr->DeviceID == 0x0000 || pciDevHdr->DeviceID == 0xffff) {
            Memory::unmap((void*)function_address);
            return;
        }

#ifdef DEBUG_PCI
        // TODO: Cache human readable information with device in device tree.
        UART::out("\r\n"
                  "      Function at 0x");
        UART::out(to_hexstring(function_address));
        UART::out(": ");
        UART::out(get_vendor_name(pciDevHdr->VendorID));
        UART::out(" / ");
        UART::out(get_device_name(pciDevHdr->VendorID, pciDevHdr->DeviceID));
        UART::out(" / ");
        UART::out(DeviceClasses[pciDevHdr->Class]);
        UART::out(" / ");
        UART::out(get_subclass_name(pciDevHdr->Class, pciDevHdr->Subclass));
        UART::out(" / ");
        UART::out(get_prog_if_name(pciDevHdr->Class, pciDevHdr->Subclass, pciDevHdr->ProgIF));
        UART::out("\r\n\r\n");
#endif /* DEBUG_PCI */

        // Class 0x01 = Mass Storage Controller
        if (pciDevHdr->Class == 0x01) {
            // Class 0x06 = Serial ATA
            if (pciDevHdr->Subclass == 0x06) {
                // ProgIF 0x01 = AHCI 1.0 Device
                if (pciDevHdr->ProgIF == 0x01) {
                    SystemDevice storageDevice(SYSDEV_MAJOR_STORAGE
                                               , SYSDEV_MINOR_AHCI_CONTROLLER
                                               , nullptr, pciDevHdr
                                               , nullptr, nullptr);
                    storageDevice.set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, true);
                    SYSTEM->add_device(storageDevice);
                }
            }
        }
    }
    
    void enumerate_device(u64 busAddress, u64 deviceNumber) {
        u64 offset = deviceNumber << 15;
        u64 device_address = busAddress + offset;
        Memory::map((void*)device_address, (void*)device_address);
        PCIDeviceHeader* pciDevHdr = reinterpret_cast<PCIDeviceHeader*>(device_address);
        if (pciDevHdr->DeviceID == 0x0000 || pciDevHdr->DeviceID == 0xffff) {
            Memory::unmap((void*)device_address);
            return;
        }
#ifdef DEBUG_PCI
        UART::out("    Device ");
        UART::out(deviceNumber);
        UART::out(":\r\n"
                  "      Address: 0x");
        UART::out(to_hexstring(device_address));
        UART::out("\r\n");
#endif /* DEBUG_PCI */
        for (u64 function = 0; function < 8; ++function)
            enumerate_function(device_address, function);
    }
    
    void enumerate_bus(u64 baseAddress, u64 busNumber) {
        u64 offset = busNumber << 20;
        u64 bus_address = baseAddress + offset;
        // Memory::map((void*)bus_address, (void*)bus_address);
#ifdef DEBUG_PCI
        UART::out("[PCI]: Bus ");
        UART::out(busNumber);
        UART::out("\r\n"
                  "  Bus Address: 0x");
        UART::out(bus_address);
        UART::out("\r\n");
#endif /* DEBUG_PCI */
        for (u64 device = 0; device < 32; ++device)
            enumerate_device(bus_address, device);
    }
    
    void enumerate_pci(ACPI::MCFGHeader* mcfg) {
        UART::out("[PCI]: Discovering devices...\r\n");
        int entries = ((mcfg->Length) - sizeof(ACPI::MCFGHeader)) / sizeof(ACPI::DeviceConfig);
#ifdef DEBUG_PCI
        UART::out("  Found ");
        UART::out(to_string(entries));
        UART::out(" MCFG entries\r\n");
        u64 systemDeviceLengthBefore = SYSTEM->devices().length();
#endif /* DEBUG_PCI */
        for (int t = 0; t < entries; ++t) {
            u64 devConAddr = (reinterpret_cast<u64>(mcfg)
                              + sizeof(ACPI::MCFGHeader)
                              + (sizeof(ACPI::DeviceConfig) * t));
            auto* devCon = reinterpret_cast<ACPI::DeviceConfig*>(devConAddr);
#ifdef DEBUG_PCI
            UART::out("    Entry ");
            UART::out(to_string(t));
            UART::out("\r\n"
                      "      Base Address: 0x");
            UART::out(to_hexstring(devCon->BaseAddress));
            UART::out("\r\n"
                      "      Start Bus: 0x");
            UART::out(to_hexstring(devCon->StartBus));
            UART::out("\r\n"
                      "      End Bus: 0x");
            UART::out(to_hexstring(devCon->EndBus));
            UART::out("\r\n");
#endif /* DEBUG_PCI */
            for (u64 bus = devCon->StartBus; bus < devCon->EndBus; ++bus) {
                enumerate_bus(devCon->BaseAddress, bus);
            }
        }
#ifdef DEBUG_PCI
        u64 systemDeviceLengthAfter = SYSTEM->devices().length();
        UART::out("[PCI]: Found ");
        UART::out(systemDeviceLengthAfter - systemDeviceLengthBefore);
        UART::out(" device(s)\r\n");
#endif /* DEBUG_PCI */
        UART::out("  \033[32mDone\033[0m\r\n\r\n");
    }
}
