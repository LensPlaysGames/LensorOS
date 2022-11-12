/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#include "devices/devices.h"

#include <acpi.h>
#include <memory/paging.h>
#include <memory/virtual_memory_manager.h>
#include <pci.h>
#include <system.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_PCI

#ifdef DEBUG_PCI
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...)
#endif

namespace PCI {
    void print_device_header(PCIDeviceHeader* pci) {
        if (pci == nullptr)
            return;

        std::print("[PCI]: Device Header at {}\n"
                   "  Vendor ID:        {}\n"
                   "  Device ID:        {}\n"
                   "  Command:          {}\n"
                   "  Status:           {}\n"
                   "  Revision ID:      {}\n"
                   "  ProgIF:           {}\n"
                   "  Subclass:         {}\n"
                   "  Class:            {}\n"
                   "  Cache Line Size:  {}\n"
                   "  Latency Timer:    {}\n"
                   "  Header Type:      {}\n"
                   "  BIST:             {}\n"
                   , (void*) pci
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
        auto* pciDevHdr = reinterpret_cast<PCIDeviceHeader*>(functionAddress);
        if (pciDevHdr->DeviceID == 0x0000 || pciDevHdr->DeviceID == 0xffff) {
            Memory::unmap((void*)functionAddress);
            return;
        }

        // TODO: Cache human readable information with device in device tree.
        DBGMSG("\n"
               "      Function at {}: {} / {} / {} / {} / {}\n"
               "\n"
               , (void*) functionAddress
               , get_vendor_name(pciDevHdr->VendorID)
               , get_device_name(pciDevHdr->VendorID, pciDevHdr->DeviceID)
               , DeviceClasses[pciDevHdr->Class]
               , get_subclass_name(pciDevHdr->Class, pciDevHdr->Subclass)
               , get_prog_if_name(pciDevHdr->Class, pciDevHdr->Subclass, pciDevHdr->ProgIF)
               );

        // Class 0x01 = Mass Storage Controller
        if (pciDevHdr->Class == 0x01) {
            // Class 0x06 = Serial ATA
            if (pciDevHdr->Subclass == 0x06) {
                // ProgIF 0x01 = AHCI 1.0 Device
                if (pciDevHdr->ProgIF == 0x01) {
                    SYSTEM->create_device<Devices::AHCIController>(*reinterpret_cast<PCIHeader0*>(pciDevHdr));
                    Memory::unmap((void*)functionAddress);
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
        auto* pciDevHdr = reinterpret_cast<PCIDeviceHeader*>(deviceAddress);
        if (pciDevHdr->DeviceID == 0x0000 || pciDevHdr->DeviceID == 0xffff) {
            Memory::unmap((void*)deviceAddress);
            return;
        }
        DBGMSG("    Device {}:\n"
               "      Address: {}\n"
               , deviceNumber
               , (void*) deviceAddress
               );
        for (u64 function = 0; function < 8; ++function)
            enumerate_function(deviceAddress, function);
    }
    
    void enumerate_bus(u64 baseAddress, u64 busNumber) {
        u64 offset = busNumber << 20;
        u64 busAddress = baseAddress + offset;
        // Memory::map((void*)bus_address, (void*)bus_address);
        DBGMSG("[PCI]: Bus {}\n"
               "  Address: {}\n"
               , busNumber
               , (void*) busAddress
               );
        for (u64 device = 0; device < 32; ++device)
            enumerate_device(busAddress, device);
    }
    
    void enumerate_pci(ACPI::MCFGHeader* mcfg) {
        std::print("[PCI]: Discovering devices...\n");
        int entries = ((mcfg->Length) - sizeof(ACPI::MCFGHeader)) / sizeof(ACPI::DeviceConfig);
        DBGMSG("  Found {} MCFG entries\n", entries);
        [[maybe_unused]] u64 systemDeviceLengthBefore = SYSTEM->Devices.size();
        for (int t = 0; t < entries; ++t) {
            u64 devConAddr = (reinterpret_cast<u64>(mcfg)
                              + sizeof(ACPI::MCFGHeader)
                              + (sizeof(ACPI::DeviceConfig) * t));
            auto* devCon = reinterpret_cast<ACPI::DeviceConfig*>(devConAddr);

            DBGMSG("    Entry {}\n"
                   "      Base Address: {}\n"
                   "      Start Bus:    {}\n"
                   "      End Bus:      {}\n"
                   , t
                   , (void*) devCon->BaseAddress
                   , devCon->StartBus
                   , devCon->EndBus
                   );

            for (u64 bus = devCon->StartBus; bus < devCon->EndBus; ++bus) {
                enumerate_bus(devCon->BaseAddress, bus);
            }
        }

        DBGMSG("[PCI]: Found {} device(s)\n"
               , SYSTEM->Devices.size() - systemDeviceLengthBefore
               );

        std::print("  \033[32mDone\033[0m\n\n");
    }
}
