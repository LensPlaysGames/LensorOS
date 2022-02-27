#include "pci.h"

#include "ahci.h"
#include "acpi.h"
#include "cstr.h"
#include "memory/heap.h"
#include "memory/virtual_memory_manager.h"
#include "uart.h"

namespace PCI {
    void enumerate_function(u64 device_address, u64 function_number) {
        u64 offset = function_number << 12;
        u64 function_address = device_address + offset;
        Memory::map((void*)function_address, (void*)function_address);
        PCIDeviceHeader* pciDevHdr = (PCIDeviceHeader*)function_address;
        if (pciDevHdr->DeviceID == 0x0000 || pciDevHdr->DeviceID == 0xffff) {
            Memory::unmap((void*)function_address);
            return;
        }

        //UART::out("  Mapped function at 0x");
        //UART::out(to_hexstring(function_address));
        //UART::out("\r\n");

        // TODO: Cache human readable information with device in device tree.
        // PRINT HUMAN READABLE INFORMATION
        //UART::out("  ");
        //UART::out(get_vendor_name(pciDevHdr->VendorID));
        //UART::out(" / ");
        //UART::out(get_device_name(pciDevHdr->VendorID, pciDevHdr->DeviceID));
        //UART::out(" / ");
        //UART::out(DeviceClasses[pciDevHdr->Class]);
        //UART::out(" / ");
        //UART::out(get_subclass_name(pciDevHdr->Class, pciDevHdr->Subclass));
        //UART::out(" / ");
        //UART::out(get_prog_if_name(pciDevHdr->Class, pciDevHdr->Subclass, pciDevHdr->ProgIF));
        //UART::out("\r\n");
        
        if (pciDevHdr->Class == 0x01) {
            // Mass Storage Controller
            if (pciDevHdr->Subclass == 0x06) {
                // Serial ATA
                if (pciDevHdr->ProgIF == 0x01) {
                    // AHCI 1.0 Device
                    AHCI::Drivers[AHCI::NumDrivers] = new AHCI::AHCIDriver(pciDevHdr);
                    ++AHCI::NumDrivers;
                }
            }
        }
    }
    
    void enumerate_device(u64 bus_address, u64 device_number) {
        u64 offset = device_number << 15;
        u64 device_address = bus_address + offset;
        Memory::map((void*)device_address, (void*)device_address);
        PCIDeviceHeader* pciDevHdr = (PCIDeviceHeader*)device_address;
        if (pciDevHdr->DeviceID == 0x0000 || pciDevHdr->DeviceID == 0xffff) {
            Memory::unmap((void*)device_address);
            return;
        }
        UART::out("  Discovered '");
        UART::out(get_device_name(pciDevHdr->VendorID, pciDevHdr->DeviceID));
        UART::out("' at ");
        UART::out(to_hexstring(device_address));
        UART::out("\r\n");
        for (u64 function = 0; function < 8; ++function)
            enumerate_function(device_address, function);
    }
    
    void enumerate_bus(u64 base_address, u64 bus_number) {
        u64 offset = bus_number << 20;
        u64 bus_address = base_address + offset;
        Memory::map((void*)bus_address, (void*)bus_address);
        PCIDeviceHeader* pciDevHdr = (PCIDeviceHeader*)bus_address;
        if (pciDevHdr->DeviceID == 0x0000 || pciDevHdr->DeviceID == 0xffff) {
            Memory::unmap((void*)bus_address);
            return;
        }

        //UART::out("  Mapped bus at 0x");
        //UART::out(to_hexstring(bus_address));
        //UART::out("\r\n");

        for (u64 device = 0; device < 32; ++device)
            enumerate_device(bus_address, device);
    }
    
    void enumerate_pci(ACPI::MCFGHeader* mcfg) {
        UART::out("[PCI]: Discovering devices...\r\n");
        int entries = ((mcfg->Length) - sizeof(ACPI::MCFGHeader)) / sizeof(ACPI::DeviceConfig);
        for (int t = 0; t < entries; ++t) {
            ACPI::DeviceConfig* devCon = (ACPI::DeviceConfig*)((u64)mcfg + sizeof(ACPI::MCFGHeader)
                                                               + (sizeof(ACPI::DeviceConfig) * t));
            for (u64 bus = devCon->StartBus; bus < devCon->EndBus; ++bus)
                enumerate_bus(devCon->BaseAddress, bus);
        }
    }
}
