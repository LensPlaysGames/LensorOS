#include <pci.h>

#include <ahci.h>
#include <acpi.h>
#include <debug.h>
#include <memory/heap.h>
#include <memory/paging.h>
#include <memory/virtual_memory_manager.h>
#include <system.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_PCI

namespace PCI {
    void print_device_header(PCIDeviceHeader* pci) {
        if (pci == nullptr)
            return;

        dbgmsg("[PCI]: Device Header at %x\r\n"
               "  Vendor ID:        %hu\r\n"
               "  Device ID:        %hu\r\n"
               "  Command:          %hu\r\n"
               "  Status:           %hu\r\n"
               "  Revision ID:      %hhu\r\n"
               "  ProgIF:           %hhu\r\n"
               "  Subclass:         %hhu\r\n"
               "  Class:            %hhu\r\n"
               "  Cache Line Size:  %hhu\r\n"
               "  Latency Timer:    %hhu\r\n"
               "  Header Type:      %hhu\r\n"
               "  BIST:             %hhu\r\n"
               , pci
               , pci->VendorID
               , pci->DeviceID
               , pci->Command
               , pci->Status
               , pci->RevisionID
               , pci->ProgIF
               , pci->Subclass
               , pci->Class
               , pci->CacheLineSize
               , pci->LatencyTimer
               , pci->HeaderType
               , pci->BIST
               );
    }

    void enumerate_function(u64 deviceAddress, u64 functionNumber) {
        u64 offset = functionNumber << 12;
        u64 functionAddress = deviceAddress + offset;
        Memory::map((void*)functionAddress, (void*)functionAddress
                    , (u64)Memory::PageTableFlag::Present
                    | (u64)Memory::PageTableFlag::ReadWrite
                    );
        PCIDeviceHeader* pciDevHdr = reinterpret_cast<PCIDeviceHeader*>(functionAddress);
        if (pciDevHdr->DeviceID == 0x0000 || pciDevHdr->DeviceID == 0xffff) {
            Memory::unmap((void*)functionAddress);
            return;
        }

#ifdef DEBUG_PCI
        // TODO: Cache human readable information with device in device tree.
        dbgmsg("\r\n"
               "      Function at %x: %s / %s / %s / %s / %s\r\n"
               "\r\n"
               , function_address
               , get_vendor_name(pciDevHdr->VendorID)
               , get_device_name(pciDevHdr->VendorID, pciDevHdr->DeviceID)
               , DeviceClasses[pciDevHdr->Class]
               , get_subclass_name(pciDevHdr->Class, pciDevHdr->Subclass)
               , get_prog_if_name(pciDevHdr->Class, pciDevHdr->Subclass, pciDevHdr->ProgIF)
               );
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
        u64 deviceAddress = busAddress + offset;
        Memory::map((void*)deviceAddress, (void*)deviceAddress
                    , (u64)Memory::PageTableFlag::Present
                    | (u64)Memory::PageTableFlag::ReadWrite
                    );
        PCIDeviceHeader* pciDevHdr = reinterpret_cast<PCIDeviceHeader*>(deviceAddress);
        if (pciDevHdr->DeviceID == 0x0000 || pciDevHdr->DeviceID == 0xffff) {
            Memory::unmap((void*)deviceAddress);
            return;
        }
#ifdef DEBUG_PCI
        dbgmsg("    Device %ull:\r\n"
               "      Address: %x\r\n"
               , deviceNumber
               , deviceAddress;
               );
#endif /* DEBUG_PCI */
        for (u64 function = 0; function < 8; ++function)
            enumerate_function(deviceAddress, function);
    }
    
    void enumerate_bus(u64 baseAddress, u64 busNumber) {
        u64 offset = busNumber << 20;
        u64 busAddress = baseAddress + offset;
        // Memory::map((void*)bus_address, (void*)bus_address);
#ifdef DEBUG_PCI
        dbgmsg("[PCI]: Bus %ull\r\n"
               "  Address: %x\r\n"
               , busNumber
               , busAddress
               );
#endif /* DEBUG_PCI */
        for (u64 device = 0; device < 32; ++device)
            enumerate_device(busAddress, device);
    }
    
    void enumerate_pci(ACPI::MCFGHeader* mcfg) {
        dbgmsg_s("[PCI]: Discovering devices...\r\n");
        int entries = ((mcfg->Length) - sizeof(ACPI::MCFGHeader)) / sizeof(ACPI::DeviceConfig);
#ifdef DEBUG_PCI
        dbgmsg("  Found %i MCFG entries\r\n", entries);
        u64 systemDeviceLengthBefore = SYSTEM->devices().length();
#endif /* DEBUG_PCI */
        for (int t = 0; t < entries; ++t) {
            u64 devConAddr = (reinterpret_cast<u64>(mcfg)
                              + sizeof(ACPI::MCFGHeader)
                              + (sizeof(ACPI::DeviceConfig) * t));
            auto* devCon = reinterpret_cast<ACPI::DeviceConfig*>(devConAddr);

#ifdef DEBUG_PCI
            dbgmsg("    Entry %i\r\n"
                   "      Base Address: %x\r\n"
                   "      Start Bus:    %x\r\n"
                   "      End Bus:      %x\r\n"
                   , t
                   , devCon->BaseAddress
                   , devCon->StartBus
                   , devCon->EndBus
                   );
#endif /* #ifdef DEBUG_PCI */

            for (u64 bus = devCon->StartBus; bus < devCon->EndBus; ++bus) {
                enumerate_bus(devCon->BaseAddress, bus);
            }
        }

#ifdef DEBUG_PCI
        dbgmsg("[PCI]: Found %ull device(s)\r\n"
               , SYSTEM->devices().length() - systemDeviceLengthBefore
               );
#endif /* #ifdef DEBUG_PCI */

        dbgmsg_s("  \033[32mDone\033[0m\r\n\r\n");
    }
}
