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
#include <dwc2_driver.h>

namespace DWC2 {
    bool Driver::init() {
        // Tell controller how to talk to physical tranceiver chips (PHY) on the
        // actual hardware.
        // We do this by modifying the GUSBCFG (global usb configuration) register
        {
            auto usb_config = global_regs()->gusbcfg;

            // disable device mode
            usb_config &= ~GUSBCFG_FORCEDEVMODE;
            // enable host mode
            usb_config |= GUSBCFG_FORCEHOSTMODE;

            auto is_phy_16bit = usb_config & GUSBCFG_PHYIF16;
            u32 turnaround_time_config_value = is_phy_16bit ? 5 : 9;

            // clear PHY selection bits
            usb_config &= ~GUSBCFG_USBTRDTIM_MASK;
            // configure PHY selection bits
            usb_config |= (turnaround_time_config_value << GUSBCFG_USBTRDTIM_BITOFF)
                          & GUSBCFG_USBTRDTIM_MASK;

            global_regs()->gusbcfg = usb_config;
        }

        // Issue full core reset
        // We do this by modifying the GRSTCTL (global reset control) register
        {
            auto reset_control = global_regs()->grstctl;

            // core: execute soft reset
            reset_control |= GRSTCTL_CSFTRST;

            global_regs()->grstctl = reset_control;

            // wait for reset to complete
            while (global_regs()->grstctl & GRSTCTL_CSFTRST)
                asm volatile("" ::: "memory");
        }

        // Configure DMA (modern memory sharing)
        // We do this by modifying the GAHBCFG (global AHB configuration) register.
        // AHB -> Advanced High-performance Bus (system bus)
        {
            auto ahb_config = global_regs()->gahbcfg;

            // set bit 0, enabling global controller core interrupts
            ahb_config |= GAHBCFG_GLBL_INTR_EN;

            // enable DMA (modern memory sharing)
            ahb_config |= GAHBCFG_DMA_EN;

            // configure burst length
            // This is the amount of internal system bus burst boundaries to do at
            // once, or something.
            ahb_config &= ~GAHBCFG_HBSTLEN_MASK;
            ahb_config |= (GAHBCFG_HBSTLEN_INCR4 << GAHBCFG_HBSTLEN_BITOFF)
                          & GAHBCFG_HBSTLEN_MASK;

            global_regs()->gahbcfg = ahb_config;
        }

        // Set up Rx and Tx FIFOs
        // We do this by modifying GRXFSIZ and GNPTXFSIZ registers
        {
            auto rxsize = 0x200; /** in 32-bit dwords **/
            global_regs()->grxfsiz = rxsize;
            global_regs()->gnptxfsiz = (rxsize << 16) | rxsize;
        }

        // Now we have the common global properties configured, we configure the
        // host configuration registers.
        {
            auto host_config = host_regs()->hcfg;

            // Configure Internal Tranceiver Clock Speed
            host_config &= ~HCFG_FSLSPCLKSEL_MASK;
            host_config |= HCFG_FSLSPCLKSEL_30_60_MHZ;

            // Enable high speed
            host_config |= HCFG_FSLSSUPP;

            host_regs()->hcfg = host_config;

            // unmask interrupts for all 16 channels
            host_regs()->haintmsk = 0xffff;
        }

        // Flush memory queues
        // We do this by modifying GRSTCTL register
        {
            // Flush receive FIFO
            {
                auto reset_control = global_regs()->grstctl;
                global_regs()->grstctl = reset_control | GRSTCTL_RXFFLSH;
                // wait for receive FIFOs to flush
                while (global_regs()->grstctl & GRSTCTL_RXFFLSH)
                    asm volatile("" ::: "memory");
            }

            // Flush all transmit FIFOs
            {
                reset_control = global_regs()->grstctl;
                reset_control &= ~GRSTCTL_TXFNUM_MASK;
                reset_control |= ((GRSTCTL_TXFNUM_ALL << GRSTCTL_TXFNUM_BITOFF)
                                  & GRSTCTL_TXFNUM_MASK)
                                 | GRSTCTL_TXFFLSH;
                global_regs()->grstctl = reset_control;
                // wait for transmit FIFOs to flush
                while (global_regs()->grstctl & GRSTCTL_TXFFLSH)
                    asm volatile("" ::: "memory");
            }
        }

        // at this point:
        // The controller core is now configured and ready for communication
        // between host and device(s).
        // Physical state changes will toggle HPRT (host port register)

        // Detect devices already plugged in (won't generate device attach
        // interrupt)
        // We do this by probing the HPRT register
        {
            auto host_port = host_regs()->hprt;

            // device is present if set
            if (host_port & HPRT_CONNSTS) {
                // device was "just plugged in" if set
                // this means the device has not yet been configured in any way by the
                // host (us) or the controller (dwc2). it simply was plugged in to the
                // physical port, and nothing has been done yet.
                if (host_port & HPRT_CONNDET) {
                    // clear bit by writing a one (W1C)
                    auto value = host_port & HPRT_W1C_SAFE;
                    value |= HPRT_CONNDET;
                    host_regs()->hprt = value;
                }

                // if we have a device connected...
                // perform port reset on connected device
                {
                    host_port = host_regs()->hprt;
                    auto port_reset_value = host_port & HPRT_W1C_SAFE;
                    port_reset_value |= HPRT_RST;
                    host_regs()->hprt = port_reset_value;
                }
                // TODO: wait 60 milliseconds
                gPIT.wait_polling(50);
                {
                    host_port = host_regs()->hprt;
                    auto port_reset_value = host_port & HPRT_W1C_SAFE;
                    port_reset_value &= ~HPRT_RST;
                    host_regs()->hprt = port_reset_value;
                }

                // wait for reset to complete
                // TODO: timeout
                while ((host_regs()->hprt & HPRT_ENA) == 0)
                    asm volatile("" ::: "memory");

                host_port = host_regs()->hprt;
                auto device_speed = (host_port & HPRT_SPD_MASK)
                                    >> HPRT_SPD_BITOFF;

                // wait 10 milliseconds
                // USB 2.0 must wait 10ms after port reset to send SETUP packet.
                gPIT.wait_polling(10);

                handle_new_device(device_speed);
            }
        }

        return true;
    }

    // Find a free host channel in the hardware layout
    int Driver::allocate_channel() {
        // DWC2 generally features between 8 to 16 channels
        auto channels = channels_base();
        for (int i = 0; i < 16; ++i) {
            // If channel is not enabled, it's free to use
            if ((channels[i].hcchar & HCCHAR_CHENA) == 0)
                return i;
        }
        // Out of channels
        return -1;
    }

    // Send an 8-byte SETUP token over DMA
    bool Driver::send_control_setup(
        int channel_number,
        u8 dev_addr,
        u32 speed,
        const UsbSetupPacket& packet) {
        auto channels = channels_base();
        volatile dwc2_hc_regs& ch = channels[channel_number];

        // Clear any lingering channel interrupts
        ch.hcint = 0xffffffff;

        // Configure Transfer Size Register for a SETUP stage
        // We are sending exactly 1 packet containing the 8-byte setup layout
        ch.hctsiz = (1u << TSIZ_PKTCNT_BITOFF)
                    | (8u << TSIZ_XFERSIZE_BITOFF)
                    | TSIZ_PID_SETUP;

        // Provide the raw physical buffer address to the DMA engine
        // Ensure your system architecture flushes or bypasses CPU cache lines here!
        ch.hcdma = uintptr_t(packet);

        // Set Max Packet Size based on device speed context (64 bytes is safe for
        // control ep0 initialization)
        u32 max_packet_size = speed == HPRT_SPD_LOW_SPEED
                                  ? 8
                                  : 64;
        // Configure Characteristics and activate channel
        auto hcchar = build_hcchar(
            true,
            false,
            dev_addr,
            0,
            HCCHAR_EPTYPE_CONTROL,
            max_packet_size);

        // Fire! Writing HCCHAR_CHENA starts the transfer
        ch.hcchar = hcchar;

        // Poll for completion
        u32 timeout = 500000;
        while (--timeout) {
            u32 interrupts = ch.hcint;

            if (interrupts & HCINTMSK_XFERCOMPL)
                return true;

            if (interrupts & (HCINTMSK_CHHLTD | HCINTMSK_AHBERR))
                return false;

            asm volatile("" ::: "memory");
        }
        // Timeout occured
        return false;
    }

    bool Driver::receive_control_data(
        int channel_number,
        u8 device_address,
        u32 speed,
        void* rx_buffer,
        u32 requested_length) {
        auto channels = channels_base();
        volatile dwc2_hc_regs& ch = channels[ch_num];

        ch.hcint = 0xffffffff;

        u32 max_packet_size = (speed == HPRT_SPD_LOW_SPEED)
                                  ? 8
                                  : 64;

        // Calculate packet boundaries needed for the size requested
        u32 packet_count = (requested_len + max_packet_size - 1)
                           / max_packet_size;

        // Control Transfers explicitly transition to DATA1 PID during the inbound
        // read payload sequence
        ch.hctsiz = (packet_count << TSIZ_PKTCNT_BITOFF)
                    | (requested_len << TSIZ_XFERSIZE_BITOFF)
                    | TSIZ_PID_DATA1;

        ch.hcdma = uintptr_t(rx_buffer);

        // Reassemble settings with the Input Direction Flag checked
        auto hcchar = build_hcchar(
            true,
            true,
            device_address,
            0,
            HCCHAR_EPTYPE_CONTROL,
            max_packet_size);

        // Fire! Trigger Read
        ch.hcchar = hcchar;

        u32 timeout = 500000;
        while (--timeout) {
            u32 interrupts = ch.hcint;

            if (interrupts & HCINTMSK_XFERCOMPL)
                return true;

            if (interrupts & (HCINTMSK_CHHLTD | HCINTMSK_AHBERR))
                return false;

            asm volatile("" ::: "memory");
        }
        return false;
    }

    bool send_control_status_out(int channel_number, uint8_t device_address, u32 speed) {
        auto channels = channels_base();
        volatile dwc2_hc_regs& ch = channels[channel_number];

        ch.hcint = 0xffffffff;

        // Status stage requires a Zero-Length Packet (XferSize = 0, PktCnt = 1)
        // Driven strictly with a DATA1 PID token layout
        ch.hctsiz = (1u << TSIZ_PKTCNT_OFF)
                    | (0u << TSIZ_XFERSIZE_OFF)
                    | TSIZ_PID_DATA1;

        // No memory buffer required for 0-byte handshake
        ch.hcdma = 0;

        u32 max_packet_size = (speed == DWC2_SPEED_LOW)
                                  ? 8
                                  : 64;

        // Output Direction Flag remains 0 (OUT transfer)
        auto hcchar = build_hcchar(
            true,
            false,
            device_address,
            0,
            HCCHAR_EPTYPE_CONTROL,
            max_packet_size);

        ch.hcchar = hcchar;  // Trigger status execution

        u32 timeout = 500000;
        while (--timeout) {
            u32 interrupts = ch.hcint;

            if (interrupts & HCINTMSK_XFERCOMPL)
                return true;

            if (interrupts & (HCINTMSK_CHHLTD | HCINTMSK_AHBERR))
                return false;

            asm volatile("" ::: "memory");
        }
        return false;
    }

    // Sets the device address from 0 to a unique identifier
    bool set_device_address(int channel_number, uint8_t current_address, uint8_t new_address, u32 speed) {
        alignas(64) UsbSetupPacket set_addr_req;
        // Host-to-Device, Standard, Device Target
        set_addr_req.bmRequestType = 0x00;
        // SET_ADDRESS
        set_addr_req.bRequest = 0x05;
        // The target address assigned
        set_addr_req.wValue = new_address;
        set_addr_req.wIndex = 0x0000;
        // No data stage for SET_ADDRESS
        set_addr_req.wLength = 0x0000;

        // Dispatch the Setup Token
        if (!send_control_setup(channel_number, current_address, speed, &set_addr_req))
            return false;

        // SET_ADDRESS has a Data Stage length of 0.
        // According to USB spec, a control transfer with NO data stage always performs
        // its Status Handshake stage in the IN direction (Device-to-Host).
        auto channels = channels_base();
        dwc2_hc_regs& ch = channels[channel_number];

        ch.hcint = 0xffffffff;
        ch.hctsiz = (1u << TSIZ_PKTCNT_OFF)
                    | (0u << TSIZ_XFERSIZE_OFF)
                    | TSIZ_PID_DATA1;
        ch.hcdma = 0;

        u32 max_packet_size = (speed == DWC2_SPEED_LOW)
                                  ? 8
                                  : 64;

        build_hcchar(true, true, current_address, 0, HCCHAR_EPTYPE_CONTROL, max_packet_size);
        u32 hcchar = HCCHAR_CHENA | HCCHAR_EPDIR_IN;

        // Fire!
        ch.hcchar = hcchar;

        u32 timeout = 500000;
        while (--timeout) {
            if (ch.hcint & (1u << 0)) return true;
            if (ch.hcint & ((1u << 1) | (1u << 4))) return false;
            asm volatile("" ::: "memory");
        }
        return false;
    }

    void Driver::handle_new_device(u32 speed) {
        // USB request packet structure
        alignas(64) UsbSetupPacket get_descriptor_cmd;
        // Inbound Data (Device to Host)
        get_descriptor_cmd.bmRequestType = 0x80;
        // GET_DESCRIPTOR Request code
        get_descriptor_cmd.bRequest = 0x06;
        // Type: 0x01 (Device Descriptor), Index: 0
        get_descriptor_cmd.wValue = 0x0100;
        get_descriptor_cmd.wIndex = 0x0000;
        // 18 bytes requested
        get_descriptor_cmd.wLength = sizeof(UsbDeviceDescriptor);

        // Find free internal hardware lane
        int channel = driver.allocate_channel();
        if (channel < 0) return;

        // Issue the command over the hardware bus to Device Address 0
        if (not driver.send_control_setup(channel, 0, &get_descriptor_cmd))
            return;

        alignas(64) UsbDeviceDescriptor device_description{};
        if (not receive_control_data(channel, 0, &device_description, sizeof(UsbDeviceDescriptor)))
            return;

        auto vendor_id = device_description.idVendor;
        auto product_id = device_description.idProduct;

        if (not send_control_status_out(channel, 0, speed))
            return;

        u8 assigned_address = 1;
        if (set_device_address(channel, 0, assigned_address, speed)) {
            // SET_ADDRESS command requires 2ms wait
            gPIT.wait_polling(2);

            // device should now respond to assigned device address
        }
    }

}  // namespace DWC2
