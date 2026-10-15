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

#include <dwc2.h>
#include <integers.h>

struct [[gnu::packed]] UsbSetupPacket {
    uint8_t bmRequestType;  // 0x80: Device-to-Host, Standard, Device target
    uint8_t bRequest;       // 0x06: GET_DESCRIPTOR
    uint16_t wValue;        // (0x01 << 8) | 0x00 -> High byte: Descriptor Type (1 = Device), Low byte: Index (0)
    uint16_t wIndex;        // 0x0000: Language ID (0 for device descriptor)
    uint16_t wLength;       // 0x0012: 18 bytes (Standard size of a USB Device Descriptor)
};

struct [[gnu::packed]] UsbDeviceDescriptor {
    uint8_t bLength;             // Size of this descriptor (18 bytes)
    uint8_t bDescriptorType;     // Descriptor Type (0x01)
    uint16_t bcdUSB;             // USB Specification Release Number (e.g., 0x0200)
    uint8_t bDeviceClass;        // Class code (e.g., 0x00 for Interface-defined, 0x09 for Hub)
    uint8_t bDeviceSubClass;     // Subclass code
    uint8_t bDeviceProtocol;     // Protocol code
    uint8_t bMaxPacketSize0;     // Maximum packet size for Endpoint 0 (Crucial!)
    uint16_t idVendor;           // Vendor ID (assigned by the USB-IF)
    uint16_t idProduct;          // Product ID (assigned by manufacturer)
    uint16_t bcdDevice;          // Device release number
    uint8_t iManufacturer;       // Index of Manufacturer string descriptor
    uint8_t iProduct;            // Index of Product string descriptor
    uint8_t iSerialNumber;       // Index of Serial Number string descriptor
    uint8_t bNumConfigurations;  // Number of possible configurations
};

struct [[gnu::packed]] UsbConfigDescriptor {
    uint8_t bLength;              // Size of this descriptor (9 bytes)
    uint8_t bDescriptorType;      // Descriptor Type (0x02)
    uint16_t wTotalLength;        // Total length of data returned (includes interfaces + endpoints)
    uint8_t bNumInterfaces;       // Number of interfaces supported by this configuration
    uint8_t bConfigurationValue;  // Value to use with SET_CONFIGURATION
    uint8_t iConfiguration;       // Index of string descriptor
    uint8_t bmAttributes;         // Configuration characteristics (Self-powered, etc.)
    uint8_t bMaxPower;            // Maximum power consumption
};

struct [[gnu::packed]] UsbInterfaceDescriptor {
    uint8_t bLength;             // Size of this descriptor (9 bytes)
    uint8_t bDescriptorType;     // Descriptor Type (0x04)
    uint8_t bInterfaceNumber;    // Number of this interface
    uint8_t bAlternateSetting;   // Value used to select this alternate setting
    uint8_t bNumEndpoints;       // Number of endpoints used by this interface
    uint8_t bInterfaceClass;     // Class code (0x08 = Mass Storage)
    uint8_t bInterfaceSubClass;  // Subclass code (0x06 = SCSI transparent command set)
    uint8_t bInterfaceProtocol;  // Protocol code (0x50 = Bulk-Only Transport / BOT)
    uint8_t iInterface;          // Index of string descriptor
};

namespace DWC2 {

class Driver {
    uintptr_t Base;

    volatile dwc2_global_regs* global_regs() {
        return ((volatile dwc2_global_regs*)(Base));
    }

    volatile dwc2_host_regs* host_regs() {
        return ((volatile dwc2_host_regs*)(Base + 0x400));
    }

    volatile dwc2_hc_regs* channels_base() {
        return ((volatile dwc2_hc_regs*)(Base + 0x500));
    }
    volatile dwc2_hc_regs* channel_regs(usz i) {
        return &channels_base()[i];
    }

    constexpr u32 build_hcchar(
        bool enabled,
        bool in_direction,
        u8 device_address,
        u8 endpoint,
        u8 transfer_type,
        u32 max_packet_size) {
        u32 hcchar = enabled ? HCCHAR_CHENA : 0;

        if (in_direction)
            hcchar |= HCCHAR_EPDIR;

        // Device Address Configuration
        hcchar |= (device_address << HCCHAR_DEVADDR_BITOFF)
                  & HCCHAR_DEVADDR_MASK;

        // Endpoint Configuration
        hcchar |= (endpoint << HCCHAR_EPNUM_BITOFF)
                  & HCCHAR_EPNUM_MASK;

        // Transfer Type
        hcchar |= (HCCHAR_EPTYPE_CONTROL << HCCHAR_EPTYPE_BITOFF)
                  & HCCHAR_EPTYPE_MASK;

        // Try up to 3 times on bus errors
        hcchar |= (3 << HCCHAR_MULTICNT_BITOFF)
                  & HCCHAR_MULTICNT_MASK;

        hcchar |= (max_packet_size << HCCHAR_MPS_BITOFF)
                  & HCCHAR_MPS_MASK;

        return hcchar;
    }

    int allocate_channel();
    bool send_control_setup(
        int channel_number,
        u8 device_address,
        u32 speed,
        const UsbSetupPacket& packet);

    bool send_control_status_out(
        int channel_number,
        uint8_t device_address,
        u32 speed);

    bool receive_control_data(
        int channel_number,
        u8 device_address,
        u32 speed,
        void* rx_buffer,
        u32 requested_length);

    bool set_device_address(
        int channel_number,
        uint8_t current_address,
        uint8_t new_address,
        u32 speed);

    void handle_new_device(u32 speed);

public:
    bool init();
};

}  // namespace DWC2
