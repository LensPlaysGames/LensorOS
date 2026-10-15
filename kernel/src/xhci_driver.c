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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

#include <xhci.h>
#include <xhci_driver.h>

bool XHCIDriver::init() {
    // TODO: host controller reset
    // TODO: wait until USBSTS.CNR (controller not ready) is clear (0)

    // Program the Max Device Slots Enabled (MaxSlotsEn) field in th e CONFIG
    // register to enable the device slots that system software is going to
    // use.

    // Program the Device Context Base Address Array Pointer (DCBAAP) register
    // with a 64-bit address pointing to where the Device Context Base Address
    // Array is located

    // Define the Command Ring Dequeue Pointer by programming the Command Ring
    // Control Register with a 64-bit address pointing to the starting address
    // of the first TRB of the Command Ring.

    // Initialize each active interrupter by:
    //
    // Defining the Event Ring
    // - Allocate and initialize the Event Ring Segment(s)
    // - Allocate the Event Ring Segment Table (ERST). Initialize ERST table
    //   entries to point to and to define the size (in TRBs) of the respective
    //   Event Ring Segment.
    // - Program the Interrupter Event Ring Segment Table Size (ERSTSZ) register
    //   with the number of segments described by the Event Ring Segment Table.
    // - Program the Interrupter Event Ring Dequeue Pointer (ERDP) register with
    //   the starting address of the first segment described by the Event Ring
    //   Segment Table.
    // - Program the Interrupter Event Ring Segment Table Base Address (ERSTBA)
    //   register with a 64-bit address pointer to where the Event Ring Segment
    //   Table is located.
    // - Note that writing the ERSTBA enables the Event Ring.
    //
    // Defining the interrupts
    // - Initializing the Interval field of the Interrupt Moderation register
    //   with the target interrupt moderation rate.
    // - Enable system bus interrupt generation by writing a 1 to the
    //   Interrupter Enable (INTE) flag of the USBCMD register.
    // - Enable the Interrupter by writing a 1 to the Interrupt Enable (IE)
    //   field of the Interrupter Management register.

    // Write USBCMD.RUN = 1 to turn the host controller ON.
    // This operation allows the xHC to begin accepting doorbell references.

    // Now we need to listen for port status change events, and initialize
    // connected devices.
    // When the xHC detects a device attach, it shall set the Current Connect
    // Status (CCS) and Connect Status Change (CSC) flags to 1. If the
    // assertion of CSC results in a 0 to 1 transition of Port Status
    // Change Event Generation (PSCEG, section 4.19.2), the xHC shall generate
    // a Port Status Change Event.
    //
    // Upon receipt of a Port Status Change Event system software evaluates
    // the Port ID field to determine the port that generated the event.
    //
    // System software then reads the PORTSC register of the port that
    // generated the event.
    // If the event was due to an attach, CCS = 1. Detach: CCS = 0.
    //
    // A USB2 protocol port requires software to reset the port to advance the
    // port to the Enabled state and a USB device from the Powered state to
    // the Default state. After an attach event, the PED and PR flags shall be
    // 0 and the PLS field shall be 7 (Polling) in the PORTSC register.
    //
    // System software shall enable the port by resetting the port (writing a
    // 1 to the PORTSC PR bit) then waiting for a Port Status Change Event due
    // to the assertion of Port Reset Change (PRC) flag.
    //
    // The completion of the port reset shall cause the PORTSC register PRC
    // and PED flags to be set (1), the PR flag to be cleared (0), and the PLS
    // field to be U0 (0). If the assertion of PRC results in a 0 to 1
    // transition of PSCEG (4.19.2), the xHC shall generate a Port Status
    // Change Event as a result of the transition of PRC. The reset operation
    // sets the USB2 device into the Default state, preparing it for a
    // SET_ADDRESS request.
    //
    // After the port successfully reaches the Enabled state, system software
    // shall obtain a Device Slot for the newly attached device using an
    // Enable Slot Command.
    //
    // After successfully obtaining a Device Slot, system software shall
    // initialize the data structures associated with the slot.
    //
    // Once the slot related data structures are initialized, system software
    // shall use an Address Device Command to assign an address to the device
    // and enable its Default Control Endpoint.
    //
    // Now that the Default Control Endpoint is fully operational, system
    // software may read the complete USB Device Descriptor and possibly the
    // Configuration Descriptors so that it can hand the device off to the
    // appropriate Class Driver(s). To read the USB descriptors, software will
    // issue USB GET_DESCRIPTOR requests through the devices’ Default Control
    // Endpoint.
    //
    // The Class Driver may then configure the Device Slot using a Configure
    // Endpoint Command as described in section 4.3.5, and configure the USB
    // Device itself by issuing a USB SET_CONFIGURATION request through the
    // devices’ Default Control Endpoint. The successful completion of both
    // operations is required to advance the state of the USB device from
    // Address to Configured and xHC Device Slot from Addressed to Configured.

    return false;
}
