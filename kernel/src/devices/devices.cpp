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

#include <ahci.h>
#include <devices/devices.h>
#include <pci.h>

Devices::AHCIController::AHCIController(PCI::PCIHeader0& hdr)
    : SystemDevice(SYSDEV_MAJOR_STORAGE, SYSDEV_MINOR_AHCI_CONTROLLER)
    , Header(hdr) {
    set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, true);
}

Devices::AHCIPort::AHCIPort(
    std::shared_ptr<AHCIController> controller,
    AHCI::PortType type,
    uint8_t i,
    AHCI::HBAPort* port
) : SystemDevice(SYSDEV_MAJOR_STORAGE, SYSDEV_MINOR_AHCI_PORT)
  , Controller(std::move(controller))
  , Driver(std::make_shared<AHCI::PortController>(type, i, port)) {
    // Search SATA devices further for partitions and filesystems.
    if (type == AHCI::PortType::SATA) set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, true);
}

Devices::GPTPartition::GPTPartition(std::shared_ptr<AHCIPort> port, GPT::PartitionEntry& part)
    : SystemDevice(SYSDEV_MAJOR_STORAGE, SYSDEV_MINOR_GPT_PARTITION)
    , Port(std::move(port))
    , Driver(std::make_shared<GPTPartitionDriver> (
        std::static_pointer_cast<StorageDeviceDriver>(Port->Driver),
        GUID(part.TypeGUID), GUID(part.UniqueGUID),
        /// TODO: Should we use part.EndLBA or part.EndLBA - part.StartLBA instead of 512?
        u64(part.StartLBA), 512
    ))
    , Partition(part) {
    set_flag(SYSDEV_MAJOR_STORAGE_SEARCH, true);
}
