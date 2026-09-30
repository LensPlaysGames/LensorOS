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

#ifndef LENSOR_OS_INTEL_HDA_H
#define LENSOR_OS_INTEL_HDA_H

/* Intel High Definition Audio 1.0 */

#include <stdint.h>
#include <x86_64/high_definition_audio_registers.h>

#include <bit>

/* Standard HDA Verb IDs */
// Intel HDA Spec 7.3.3.31
#define HDA_VERB_GET_PARAMETER 0xf00
// Intel HDA Spec 7.3.3.31
#define HDA_VERB_GET_CONFIG_DEFAULT 0xf1c

struct PinConfig {
    uint32_t data;

    // 00b The Port Complex is connected to a jack (1/8", ATAPI, etc.).
    // 01b No physical connection for Port.
    // 10b A fixed function device (integrated speaker, integrated mic, etc.)
    //     is attached.
    // 11b Both a jack and an internal device are attached. The information
    //     provided in all other fields refers to the integrated device. The PD
    //     pin will reflect the status of the jack; the user will need to be
    //     queried to figure out what it is.
    uint32_t port_connectivity() const {
        return (data >> 30) & 0b11;
    }

    // 0x0 -- Line Out
    // 0x1 -- Speaker
    // 0x2 -- Headphone Out
    // 0x3 -- CD
    // 0x4 -- SPDIF Out
    // 0x5 -- Digital Other Out
    // 0x6 -- Modem Line Side
    // 0x7 -- Modem Handset Side
    // 0x8 -- Line In
    // 0x9 -- AUX
    // 0xa -- Mic In
    // 0xb -- Telephony
    // 0xc -- SPDIF In
    // 0xd -- Digital Other In
    // 0xe -- Reserved
    // 0xf -- Other
    uint32_t default_device() const {
        return (data >> 30) & 0xf;
    }

    bool is_simple_out() {
        return port_connectivity() != 1
               and default_device() <= 2;
    }
};

/* Parameter Payloads for Verb 0xf00 */
#define HDA_PARAM_SUBORDINATE_NODE_COUNT 0x04
#define HDA_PARAM_FUNCTION_GROUP_TYPE 0x05
#define HDA_PARAM_AUDIO_WIDGET_CAPABILITIES 0x09
#define HDA_PARAM_PIN_CAPABILITIES 0x0c
#define HDA_PARAM_CONN_LIST_LENGTH 0x0e

/* Function Group Types (Response to PARAM_FUNCTION_GROUP_TYPE) */
#define HDA_FUNC_GROUP_TYPE_AUDIO 0x01
#define HDA_FUNC_GROUP_TYPE_MODEM 0x02

/* Widget Types (Extracted from PARAM_AUDIO_WIDGET_CAPABILITIES bits 23:20) */
#define HDA_WIDGET_TYPE_AUDIO_OUTPUT 0x0    /**< DAC */
#define HDA_WIDGET_TYPE_AUDIO_INPUT 0x01    /**< ADC */
#define HDA_WIDGET_TYPE_AUDIO_MIXER 0x02    /**< Summer/Mixer */
#define HDA_WIDGET_TYPE_AUDIO_SELECTOR 0x03 /**< Multiplexer */
#define HDA_WIDGET_TYPE_PIN_COMPLEX 0x04    /**< Physical Jack/Internal Speaker/Mic */
#define HDA_WIDGET_TYPE_POWER_STATE 0x05    /**< Power Control */
#define HDA_WIDGET_TYPE_VOLUME_KNOB 0x06    /**< Physical volume wheel */
#define HDA_WIDGET_TYPE_BEEP_GENERATOR 0x07 /**< Internal Beep */
#define HDA_WIDGET_TYPE_VENDOR_DEFINED 0x0f

/* Helper Macros for Parsing Responses */
#define HDA_PARAM_NODE_START(resp) (((resp) >> 16) & 0xff)
#define HDA_PARAM_NODE_COUNT(resp) ((resp) & 0xff)
#define HDA_PARAM_WIDGET_TYPE(resp) (((resp) >> 20) & 0x0f)

/* Verbs for Routing and Path Selection */
#define HDA_VERB_GET_CONN_LIST_LEN 0xf00 /* Use with parameter 0x0e */
#define HDA_VERB_GET_CONN_LIST_ENTRY 0xf02
#define HDA_VERB_SET_CONN_SELECT 0x701
#define HDA_VERB_SET_POWER_STATE 0x705
// Intel HDA 7.3.3.11 Converter Stream, Channel
// Stream is in bits 7:4
// Channel is in bits 3:0
#define HDA_VERB_SET_CONV_STREAM_CHAN 0x706
#define HDA_VERB_SET_CONV_FMT 0x2

/* Pin Widget Control Verbs (Crucial to turn the jack on) */
#define HDA_VERB_SET_PIN_WIDGET_CTL 0x707
#define HDA_PIN_CTL_ENABLE_OUTPUT 0x40 /* Bit 6: Enables the output amplifier */
#define HDA_PIN_CTL_ENABLE_HP 0x80     /* Bit 7: Enables headphone amplifier cap */

/* Audio Widget Control (Mute/Volume) */
#define HDA_VERB_SET_AMP_GAIN_MUTE 0x3
/* Clear the mute bit to let sound pass */
#define HDA_AMP_MUTE_ENABLE ((uint32_t)1 << 7)
#define HDA_AMP_PARAM_SET_RIGHT (((uint32_t)1 << 12) >> 8)
#define HDA_AMP_PARAM_SET_LEFT (((uint32_t)1 << 13) >> 8)
#define HDA_AMP_PARAM_SET_INPUT (((uint32_t)1 << 14) >> 8)
#define HDA_AMP_PARAM_SET_OUTPUT (((uint32_t)1 << 15) >> 8)

#define HDA_PIN_CAP_HEADPHONE ((uint32_t)1 << 3)
#define HDA_PIN_CAP_OUTPUT ((uint32_t)1 << 4)
#define HDA_PIN_CAP_INPUT ((uint32_t)1 << 5)

// Global Controller Registers (Offsets from BAR0 base)
typedef struct IntelHDAGlobalRegs {
    uint16_t gcap;    // 0x00: Global Capabilities
    uint8_t vmin;     // 0x02: Minor Version
    uint8_t vmaj;     // 0x03: Major Version
    uint16_t outpay;  // 0x04: Output Payload Capability
    uint16_t inpay;   // 0x06: Input Payload Capability
    uint32_t gctl;    // 0x08: Global Control
    uint16_t wakeen;  // 0x0C: Wake Enable
    /* A 1 in a given bit position indicates that a codec at that associated
     * address is present. For instance, a value of 0x05 means that there are
     * codecs with addresses 0 and 2 attached to the link.     */
    uint16_t statests;  // 0x0E: State Change Status
    uint16_t gsts;      // 0x10: Global Status

    uint16_t output_stream_count() volatile const {
        return (gcap >> 12) & 0xf;
    }
    uint16_t input_stream_count() volatile const {
        return (gcap >> 8) & 0xf;
    }
    uint16_t bidi_stream_count() volatile const {
        return (gcap >> 3) & 0x1f;
    }
    uint16_t serial_data_out_signal_count() volatile const {
        switch ((gcap >> 1) & 0b11) {
            case 0b00:
                return 1;
            case 0b01:
                return 2;
            case 0b10:
                return 4;
            default:
                break;
        }
        return 0;
    }
    bool is64() volatile const {
        return gcap & 1;
    }

    // When the controller hardware is ready to begin operation, it will
    // report a 1 in this bit. Software must read a 1 from this bit before
    // accessing any controller registers. The CRST# bit defaults to a 0 after
    // hardware reset, therefore software needs to write a 1 to this bit to
    // begin operation.
    // Note that the CORB/RIRB RUN bits and all Stream RUN bits must be
    // verified cleared to 0 before CRST# is written to 0 (asserted) in order
    // to assure a clean restart.
    bool controller_reset_bit() volatile const {
        return gctl & 1;
    }
} IntelHDAGlobalRegs;

// Layout for a single DMA Stream Engine (e.g., Output Stream 1)
// On standard Intel HDA, Output Stream 1 typically begins at offset 0x0E0 + (0 * 0x20)
typedef struct IntelHDAStreamRegs {
    volatile uint32_t ctl;       // 0x00: Stream Control (3 bytes) + Status (1 byte)
    volatile uint32_t lpib;      // 0x04: Link Position In Buffer
    volatile uint32_t cbl;       // 0x08: Cyclic Buffer Length (Total size of DMA buffer pool)
    volatile uint16_t lvi;       // 0x0c: Last Valid Index (Number of BDL entries minus 1)
    volatile uint16_t reserved;  // 0x0e: Reserved
    volatile uint16_t fifos;     // 0x10: FIFO Size
    volatile uint16_t fmt;       // 0x12: Stream Format
    volatile uint64_t bdlp;      // 0x14: Buffer Descriptor List Pointer Lower 32-bits

    uint32_t control() const volatile {
        return ctl & 0x00ffffff;
    };
    uint8_t status() const volatile {
        return ((ctl >> 24) & 0xff);
    };

    static constexpr uint32_t build_ctl(uint32_t control, uint8_t status = 0) {
        return (control & 0xffffff) | (uint32_t(status) << 24);
    }

    void control(uint32_t new_value) volatile {
        auto new_ctl = build_ctl(new_value, status());
        std::print("  control: {:#x} -> {:#x}\n", (uint32_t)ctl, new_ctl);
        ctl = new_ctl;
    };
    // Bits 2, 3, and 4 of status are write-1-to-clear
    void status_clear() volatile {
        ctl = build_ctl(control(), 0b111 << 2);
    };

    void stream_reset() volatile {
        // Set SRST (Bit 0 of control block -> physical bit 8)
        auto current = control();
        control(current | HDA_SD_REG_CTL_RESET);

        // Tiny delay to give hardware time to perform reset (not required).
        for (volatile auto i = 0u; i < 10000u; i += 1)
            asm volatile("pause");

        // Clear SRST to bring it back online
        control(control() & ~HDA_SD_REG_CTL_RESET);

        for (volatile auto i = 0u; i < 10000u; i += 1)
            asm volatile("pause");
    }
} IntelHDAStreamRegs;
static_assert(
    offsetof(IntelHDAStreamRegs, fifos) == HDA_SD_REG_FIFOD,
    "Intel HDA Stream FIFO Register Offset");
static_assert(
    offsetof(IntelHDAStreamRegs, bdlp) == HDA_SD_REG_BDPL,
    "Intel HDA Stream Buffer Descriptor List Register Offset");

static_assert(
    sizeof(IntelHDAStreamRegs) == 0x20,
    "Intel HDA Stream Registers are 0x20 bytes");

// Buffer Descriptor entry layout (Must be 128-bit aligned in memory)
struct alignas(16) IntelHDABdlEntry {
    uint64_t address;
    uint32_t length;  // Number of bytes in this specific buffer fragment
    uint32_t ioc;     // Bit 0 = Interrupt on Completion (IOC) flag
};
static_assert(sizeof(IntelHDABdlEntry) == 16, "[HDA]: BDL Entries are 16 bytes");

struct HDAController {
    bool init();

    void set_base(uintptr_t base) { Base = base; }

    void handle_interrupt();

   private:
    // spin/busy-wait until response recieved.
    uint32_t send_command(
        uint32_t verb,
        uint32_t param,
        uint32_t data,
        uint32_t node,
        uint32_t codec);

    bool initialize_corb();
    bool initialize_rirb();

    // BAR0 address
    uintptr_t Base{0};

    // Command Rings
    volatile uint32_t* CORB{0};
    volatile uint64_t* RIRB{0};
    uintptr_t CORBEntryCount{0};
    uintptr_t RIRBEntryCount{0};

    // Audio Sample Buffers
    uintptr_t BufferCount{0};
    uintptr_t CurrentBufferIndex{0};

    volatile IntelHDAGlobalRegs* global_regs() { return (volatile IntelHDAGlobalRegs*)Base; };

    uint32_t wall_clock() {
        return *(volatile uint32_t*)(Base + HDA_REG_WALCLK);
    }

    // Bottom bits 0 through 6 are hard-wired to zero for forced 128-byte
    // alignment.
    volatile uint32_t* corb_lower() {
        return (volatile uint32_t*)(Base + HDA_REG_CORBLBASE);
    }
    volatile uint32_t* corb_upper() {
        return (volatile uint32_t*)(Base + HDA_REG_CORBUBASE);
    }
    volatile uint16_t* corb_wp() {
        return (volatile uint16_t*)(Base + HDA_REG_CORBWP);
    }
    volatile uint16_t* corb_rp() {
        return (volatile uint16_t*)(Base + HDA_REG_CORBRP);
    }
    volatile uint8_t* corb_control() {
        return (volatile uint8_t*)(Base + HDA_REG_CORBCTL);
    }
    volatile uint8_t* corb_status() {
        return (volatile uint8_t*)(Base + HDA_REG_CORBSTS);
    }
    volatile uint8_t* corb_size() {
        return (volatile uint8_t*)(Base + HDA_REG_CORBSIZE);
    }

    // Bottom bits 0 through 6 are hard-wired to zero for forced 128-byte
    // alignment.
    volatile uint32_t* rirb_lower() {
        return (volatile uint32_t*)(Base + HDA_REG_RIRBLBASE);
    }
    volatile uint32_t* rirb_upper() {
        return (volatile uint32_t*)(Base + HDA_REG_RIRBUBASE);
    }
    volatile uint16_t* rirb_wp() {
        return (volatile uint16_t*)(Base + HDA_REG_RIRBWP);
    }
    volatile uint16_t* rirb_count() {
        return (volatile uint16_t*)(Base + HDA_REG_RINTCNT);
    }
    volatile uint8_t* rirb_control() {
        return (volatile uint8_t*)(Base + HDA_REG_RIRBCTL);
    }
    volatile uint8_t* rirb_status() {
        return (volatile uint8_t*)(Base + HDA_REG_RIRBSTS);
    }
    volatile uint8_t* rirb_size() {
        return (volatile uint8_t*)(Base + HDA_REG_RIRBSIZE);
    }

    volatile uint32_t* dma_position_lower() {
        return (volatile uint32_t*)(Base + HDA_REG_DPIBLBASE);
    }
    volatile uint32_t* dma_position_upper() {
        return (volatile uint32_t*)(Base + HDA_REG_DPIBUBASE);
    }
};

extern HDAController* hda_interrupt_handler_controller;

#endif  // LENSOR_OS_INTEL_HDA_H
