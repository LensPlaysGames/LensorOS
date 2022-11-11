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

#ifndef LENSOROS_DEVICES_H
#define LENSOROS_DEVICES_H

#include <gpt.h>
#include <pci.h>
#include <storage/device_drivers/gpt_partition.h>
#include <storage/device_drivers/port_controller.h>
#include <system.h>

namespace Devices {

struct AHCIController : SystemDevice {
    PCI::PCIHeader0 Header;
    AHCIController(PCI::PCIHeader0& hdr);
};

struct AHCIPort : SystemDevice {
    std::shared_ptr<AHCIController> Controller;
    std::shared_ptr<AHCI::PortController> Driver;

    AHCIPort(std::shared_ptr<AHCIController> controller, AHCI::PortType type, uint8_t i, AHCI::HBAPort* port);
};

struct GPTPartition : SystemDevice {
    std::shared_ptr<AHCIPort> Port;
    std::shared_ptr<GPTPartitionDriver> Driver;
    GPT::PartitionEntry Partition;

    GPTPartition(std::shared_ptr<AHCIPort> port, GPT::PartitionEntry& part);
};

}

#endif // LENSOROS_DEVICES_H
