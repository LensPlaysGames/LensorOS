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

#include <event.h>
#include <memory/physical_memory_manager.h>
#include <memory/types.h>
#include <memory/virtual_memory_manager.h>
#include <stdint.h>
#include <system.h>
#include <x86_64/high_definition_audio.h>

#include <print>
#include <vector>

constexpr inline uint32_t form_command(uint32_t verb, uint32_t param, uint32_t data, uint32_t node, uint32_t codec) {
    uint32_t verb_payload = 0;

    // > There are two types of verbs: those with 4-bit identifiers and 16 bits
    // > of data payload and those with 12-bit identifiers and 8 bits of
    // > payload. Because of the limited encoding space for the 4-bit
    // > identifiers, they are used sparingly for operations which need to
    // > convey data to the codec in 16-bit payloads. The values 0x7 and 0xF are
    // > not legal values for 4-bit verbs, as they select the extended 12-bit
    // > identifiers.
    if ((verb & 0xf00) == 0xf00 or (verb & 0xf00) == 0x700) {
        if (data) {
            std::print("[HDA]: Boi you dun fucked up; 0x7xx and 0xfxx verbs do not use data parameter; it is simply the bottom 8 bits of param\n");
        }
        // Extended 12-bit verb layout (e.g., 0xf00, 0xf1c)
        // Verb takes bits [19:8], parameter/data takes bits [7:0]
        verb_payload = ((verb & 0xfff) << 8) | (param & 0xff);
    }
    else {
        // Standard 4-bit verb layout (e.g., 0x3 for Amp Gain)
        // Verb takes bits [19:16], parameter takes bits [15:8], data takes bits [7:0]
        verb_payload = ((verb & 0xf) << 16) | ((param & 0xff) << 8) | (data & 0xff);
    }

    return ((codec & 0xf) << 28)
           | ((node & 0xff) << 20)
           | (verb_payload & 0xfffff);
}

uint32_t HDAController::send_command(uint32_t verb, uint32_t param, uint32_t data, uint32_t node, uint32_t codec) {
    // std::print("[HDA]: CORB command: verb={:#x} param={:#x} data={:#x} node={:#x} codec={:#x}\n", verb, param, data, node, codec);

    const uint32_t corb_command = form_command(
        verb,
        param,
        data,
        node,
        codec);

    // std::print("  command: {:#08x}\n", corb_command);

    // --- WRITE TO CORB ---
    // Read the current hardware write pointer (Offset 0x4A)
    auto current_wp = *corb_wp();

    // Calculate the next index inside the ring buffer
    auto next_wp = (current_wp + 1) % CORBEntryCount;

    // std::print("  current wp: {}  next wp: {}\n", current_wp, next_wp);

    // Place the constructed command inside the DMA memory block
    CORB[next_wp] = corb_command;

    uint16_t expected_rirbwp = (*rirb_wp() + 1) % RIRBEntryCount;

    // Ensure command is actually written before updating hardware write
    // pointer to look at this memory.
    asm volatile("mfence" ::: "memory");

    // Update the hardware Write Pointer register to trigger hardware
    // to process the command.
    *corb_wp() = next_wp;

    // Ensure write pointer is actually written before spinning.
    (void)*corb_wp();
    asm volatile("mfence" ::: "memory");

    // --- WAIT FOR RIRB RESPONSE ---
    // Implement a timeout loop so a dead codec doesn't hang bootup
    uint32_t timeout = 4000000;
    while (*corb_rp() != next_wp and timeout > 0) {
        --timeout;
        asm volatile("pause");
    }
    if (timeout == 0) {
        std::print("[HDA]:ERROR: Command timeout waiting for CORB. Verb: {:#x}\n", verb);
        std::print("  gctl:{:#x} gsts:{:#x}\n", (uint32_t)global_regs()->gctl, (uint32_t)global_regs()->gsts);
        std::print(
            "  CORB wp:{} rp:{} ctl:{} size:{} status:{}\n",
            (int)*corb_wp(),
            (int)*corb_rp(),
            (int)*corb_control(),
            (int)*corb_size(),
            (int)*corb_status());
        std::print(
            "  RIRB wp:{} rp:{} ctl:{} size:{} status:{}\n",
            (int)*rirb_wp(),
            expected_rirbwp,
            (int)*rirb_control(),
            (int)*rirb_size(),
            (int)*rirb_status());
        // failure sentinel
        return 0xffffffff;
    }

    timeout = 2000000;
    while (*rirb_wp() != expected_rirbwp and timeout > 0) {
        --timeout;
        asm volatile("pause");
    }
    if (timeout == 0) {
        std::print("[HDA]:ERROR: Command timeout waiting for RIRB. Verb: {:#x}\n", verb);
        // failure sentinel
        return 0xffffffff;
    }

    // --- READ RESPONSE ---
    // Grab the full 64-bit frame from our physical DMA ring
    auto response = RIRB[expected_rirbwp];

    // Clear the response interrupt bit in RIRBSTS (Offset 0x5D) to acknowledge processing
    // Clear RINTFL (bit 0) if set (write 1 to clear)
    // TODO: Clear RIRBOIS (bit 2) if set
    if (*rirb_status() & 1)
        *rirb_status() = 1;

    for (volatile int i = 0; i < 1000; i += 1) {
        asm volatile("pause");
    };

    // Return the lower 32-bit payload data containing the verb return value
    return (uint32_t)response;
}

bool HDAController::initialize_corb() {
    // Stop CORB
    *corb_control() = *corb_control() & ~0b11;
    // Wait for CORB to stop
    uint32_t timeout = 2000000;
    while (*corb_control() & 0b10 and timeout > 0) {
        --timeout;
        asm volatile("pause");
    }
    if (timeout == 0) {
        std::print("[HDA]:  CORB Initialization Failed: timeout expired waiting for CORB DMA to stop\n");
        return false;
    }

    const auto CORB_physical = Memory::TO_FRAME_POINTER(
        PhysicalKernelAddress(CORB));
    *corb_lower() = (uintptr_t)CORB_physical;
    *corb_upper() = (uintptr_t)CORB_physical >> 32;

    // Reset CORB Read Pointer
    // > CORB Read Pointer Reset (CORBRPRST): Software writes a 1 to this bit to
    // > reset the CORB Read Pointer to 0 and clear any residual pre-fetched
    // > commands in the CORB hardware buffer within the controller. The
    // > hardware will physically update this bit to 1 when the CORB pointer
    // > reset is complete. Software must read a 1 to verify that the reset
    // > completed correctly. Software must clear this bit back to 0, by writing
    // > a 0, and then read back the 0 to verify that the clear completed
    // > correctly.
    const uint16_t reset_rp_bit = ((uint16_t)1) << 15;
    *corb_rp() = reset_rp_bit;
    // Wait for read pointer reset bit to update
    timeout = 2000000;
    while ((*corb_rp() & reset_rp_bit) == 0 and timeout > 0) {
        --timeout;
        asm volatile("pause");
    }
    if (timeout == 0) {
        std::print("[HDA]: CORB Initialization Failed: timeout expired waiting for CORB Read Pointer Reset bit to be set to one after writing\n");
        return false;
    }

    // Clear the reset bit back to 0
    *corb_rp() = 0;
    timeout = 2000000;
    while ((*corb_rp() & reset_rp_bit) and timeout > 0) {
        --timeout;
        asm volatile("pause");
    }
    if (timeout == 0) {
        std::print("[HDA]: CORB Initialization Failed: timeout expired waiting for CORB Read Pointer Reset bit to clear back to zero after writing\n");
        return false;
    }

    // Bits [7:4] RIRB Size
    // 0b0001 16B   = 2 entries
    // 0b0010 128B  = 16 entries
    // 0b0100 2048B = 256 Entries
    // 0b1000 Reserved
    // This implemented as a bit mask; for example, if the controller supported two
    // entries and 256 entries, this register would be Read Only 0101b.
    // There is no requirement to support more than one RIRB Size.
    auto size_tag = (*corb_size() >> 4) & 0xf;
    uint8_t size_value = 0;
    if (size_tag & 0b100) {
        CORBEntryCount = 256;
        size_value = 0b10;
    }
    else if (size_tag & 0b10) {
        CORBEntryCount = 16;
        size_value = 0b01;
    }
    else {
        CORBEntryCount = 2;
        size_value = 0b00;
    }
    *corb_size() = size_value;

    // Start CORB DMA
    *corb_control() = *corb_control() | 0b10;
    std::print("[HDA] CORB Initialized\n");

    return true;
}

bool HDAController::initialize_rirb() {
    // Stop RIRB
    *rirb_control() = *rirb_control() & ~0b11;
    // Wait for RIRB to stop
    uint32_t timeout = 2000000;
    while (*rirb_control() & 0b10 and timeout > 0) {
        --timeout;
        asm volatile("pause");
    }
    if (timeout == 0) {
        std::print("[HDA]: RIRB Initialization Failed: timeout expired waiting for RIRB DMA to stop\n");
        return false;
    }

    const auto RIRB_physical = Memory::TO_FRAME_POINTER(
        PhysicalKernelAddress(RIRB));
    *rirb_lower() = (uintptr_t)RIRB_physical;
    *rirb_upper() = (uintptr_t)RIRB_physical >> 32;

    // Reset RIRB Write Pointer
    // NOTE: not as complex as CORB, as this bit always reads zero.
    const uint16_t reset_wp_bit = ((uint16_t)1) << 15;
    *rirb_wp() = reset_wp_bit;

    auto size_tag = (*rirb_size() >> 4) & 0xf;
    uint8_t size_value = 0;
    if (size_tag & 0b100) {
        RIRBEntryCount = 256;
        size_value = 0b10;
    }
    else if (size_tag & 0b10) {
        RIRBEntryCount = 16;
        size_value = 0b01;
    }
    else {
        RIRBEntryCount = 2;
        size_value = 0b00;
    }
    *rirb_size() = size_value;

    // Ensure non-zero value here
    *rirb_count() = 1;

    // Start RIRB DMA
    // NOTE: QEMU has bugs/quirks with regards to internal state breaking when
    // RINTCTL bit is not set within RIRBCTL. We set it during initialization
    // no matter what.
    *rirb_control() = *rirb_control()
                      | HDA_REG_RIRBCTL_DMA_ENABLE
                      | HDA_REG_RIRBCTL_INTERRUPT_ENABLE;
    std::print("[HDA] RIRB Initialized\n");

    return true;
}

HDAController* hda_interrupt_handler_controller{};

bool HDAController::init() {
    // Base address must be set during PCI enumeration
    if (not Base) return false;

    hda_interrupt_handler_controller = this;

    uint32_t timeout = 2000000;

    // Put controller into reset, and wait for controller to acknowledge it.
    std::print("[HDA]: Performing controller reset\n");
    global_regs()->gctl = global_regs()->gctl & ~(uint32_t)1;
    while ((global_regs()->gctl & 1) != 0 and timeout > 0) {
        --timeout;
        asm volatile("pause");
    };
    if (timeout == 0) {
        std::print("[HDA]: Timed out waiting for reset bit to clear (enter reset)\n");
        return false;
    }

    std::print("[HDA]: Controller resetting...\n");
    // Give the hardware a moment in the reset state (HDA spec recommends a
    // short delay)
    for (volatile int i = 0; i < 100000; i += 1) {
        asm volatile("pause");
    }

    // Write CRST bit to move controller out of reset state.
    global_regs()->gctl = global_regs()->gctl | 1;

    // Wait for controller to actually start up.
    // > ... after taking the controller out of reset, the software should wait
    // > until CRST is read as 1 before continuing
    timeout = 2000000;
    while ((global_regs()->gctl & 1) == 0 and timeout > 0) {
        --timeout;
        asm volatile("pause");
    };
    if (timeout == 0) {
        std::print("[HDA]: Timed out waiting for reset bit to set (exit reset)\n");
        return false;
    }

    // Give the hardware a moment out of the reset state.
    // The HDA specification states you must wait a minimum of 521
    // microseconds after bringing the link out of reset before talking to a
    // codec to allow its internal PLLs to lock.
    for (volatile int i = 0; i < 100000; i += 1) {
        asm volatile("pause");
    }

    std::print("[HDA]: Started up controller\n");

    // Initialize CORB and RIRB buffers, so that we can send commands and
    // receive responses.
    CORB = (decltype(CORB))Memory::request_page();
    // Skip past maximum size of CORB to place RIRB
    RIRB = decltype(RIRB)(CORB + 256);

    Memory::map(
        (void*)CORB,
        (void*)Memory::TO_FRAME_POINTER(PhysicalKernelAddress(CORB)).address,
        (u64)Memory::PageTableFlag::Present
            | (u64)Memory::PageTableFlag::ReadWrite
            | (u64)Memory::PageTableFlag::CacheDisabled
            | (u64)Memory::PageTableFlag::WriteThrough);

    initialize_corb();
    initialize_rirb();

    // For every "codec" that has a bit set in STATESTS
    for (uintptr_t codec_i = 0; codec_i < 16; ++codec_i) {
        const bool codec_present = global_regs()->statests & (1 << codec_i);
        if (not codec_present) continue;
        std::print("[HDA]: Codec {} is present\n", codec_i);

        // Find any Audio Function Groups (AFGs) within the codec. An AFG is
        // basically just a semantic group of audio devices, like a DAC and Pin
        // Complex (output jack), that can talk to each other.

        // Codecs always "start" at node zero (0x00).
        // Query Node 0x00 to find out how many function groups exist and what the
        // starting Node ID is.
        auto codec_query_result = send_command(
            HDA_VERB_GET_PARAMETER,
            HDA_PARAM_SUBORDINATE_NODE_COUNT,
            0,
            0,
            codec_i);
        if (codec_query_result == 0xffffffff) {
            std::print("  failed to send query command to codec {}\n", codec_i);
            continue;
        }
        const uint32_t function_group_count = HDA_PARAM_NODE_COUNT(codec_query_result);
        const uint32_t function_group_start_i = HDA_PARAM_NODE_START(codec_query_result);

        // small delay for codec to wake
        for (volatile int i = 0; i < 10000; i += 1) {
            asm volatile("pause");
        };

        std::print("  function group count: {}  start: {}\n", function_group_count, function_group_start_i);

        // For every AFG passed as a response, verify it's *actually* an AFG...
        for (uint32_t function_group_i = function_group_start_i;
             function_group_i < function_group_start_i + function_group_count;
             ++function_group_i) {
            std::print("  powering on function group {}\n", function_group_i);
            send_command(HDA_VERB_SET_POWER_STATE, 0, 0, function_group_i, codec_i);

            // Node type verification
            std::print("  querying function group {} for type\n", function_group_i);
            auto function_group_type_query_result = send_command(
                HDA_VERB_GET_PARAMETER,
                HDA_PARAM_FUNCTION_GROUP_TYPE,
                0,
                function_group_i,
                codec_i);
            function_group_type_query_result &= 0xff;
            if (function_group_type_query_result != HDA_FUNC_GROUP_TYPE_AUDIO) continue;
            std::print("  Audio Function Group at {}\n", function_group_i);

            // Query audio function group node for it's children start and count; this
            // will get us to the actual widgets, finally.

            auto function_group_query_result = send_command(
                HDA_VERB_GET_PARAMETER,
                HDA_PARAM_SUBORDINATE_NODE_COUNT,
                0,
                function_group_i,
                codec_i);
            const uint32_t widget_count = HDA_PARAM_NODE_COUNT(function_group_query_result);
            const uint32_t widget_start_i = HDA_PARAM_NODE_START(function_group_query_result);

            auto widget_types = std::vector<u8>(widget_count, u8(-1));

            for (uint32_t widget_i = widget_start_i; widget_i < widget_start_i + widget_count; ++widget_i) {
                std::print("  Widget at {}\n", widget_i);
                auto widget_capabilities_query_result = send_command(
                    HDA_VERB_GET_PARAMETER,
                    HDA_PARAM_AUDIO_WIDGET_CAPABILITIES,
                    0,
                    widget_i,
                    codec_i);
                auto widget_type = HDA_PARAM_WIDGET_TYPE(widget_capabilities_query_result);
                widget_types[widget_i] = widget_type;
                switch (widget_type) {
                    case HDA_WIDGET_TYPE_AUDIO_OUTPUT: {
                        std::print("  output\n");
                    } break;
                    case HDA_WIDGET_TYPE_AUDIO_INPUT: {
                        std::print("  input\n");
                    } break;
                    case HDA_WIDGET_TYPE_AUDIO_MIXER: {
                        std::print("  mixer\n");
                    } break;
                    case HDA_WIDGET_TYPE_AUDIO_SELECTOR: {
                        std::print("  selector\n");
                    } break;
                    case HDA_WIDGET_TYPE_PIN_COMPLEX: {
                        std::print("  pin complex\n");

                        // Power up pin widget
                        std::print("  ... powering pin complex on\n");
                        send_command(HDA_VERB_SET_POWER_STATE, 0, 0, widget_i, codec_i);
                        std::print("  powered on\n");

                        auto pin_complex_query_result = send_command(
                            HDA_VERB_GET_CONFIG_DEFAULT,
                            0,
                            0,
                            widget_i,
                            codec_i);
                        auto pin_complex_config = PinConfig(pin_complex_query_result);
                        switch (pin_complex_config.port_connectivity()) {
                            case 0b00: {
                                std::print("  jack\n");
                            } break;
                            case 0b01: {
                                std::print("  not connected\n");
                            } break;
                            case 0b10: {
                                std::print("  fixed function device\n");
                            } break;
                            case 0b11: {
                                std::print("  jack + fixed function device\n");
                            } break;
                        }
                        switch (pin_complex_config.default_device()) {
                            case 0x0: {
                                std::print("  line-out\n");
                            } break;
                            case 0x1: {
                                std::print("  speaker\n");
                            } break;
                            case 0x2: {
                                std::print("  headphones\n");
                            } break;
                            case 0x3: {
                                std::print("  cd\n");
                            } break;
                            case 0x4: {
                                std::print("  spdif-out\n");
                            } break;
                            case 0x5: {
                                std::print("  digital-out\n");
                            } break;
                            case 0x6: {
                                std::print("  modem-line\n");
                            } break;
                            case 0x7: {
                                std::print("  modem-handset\n");
                            } break;
                            case 0x8: {
                                std::print("  line-in\n");
                            } break;
                            case 0x9: {
                                std::print("  aux\n");
                            } break;
                            case 0xa: {
                                std::print("  mic\n");
                            } break;
                            case 0xb: {
                                std::print("  telephony\n");
                            } break;
                            case 0xc: {
                                std::print("  spdif-in\n");
                            } break;
                            case 0xd: {
                                std::print("  digital-in\n");
                            } break;
                            case 0xe: {
                                std::print("  reserved\n");
                            } break;

                            default:
                            case 0xf: {
                                std::print("  other\n");
                            } break;
                        }

                        if (not pin_complex_config.is_simple_out())
                            break;

                        // Check if the pin can actually output audio (Pin Capabilities bit 4)
                        std::print("  checking pin capabilities\n");
                        auto pin_capabilities = send_command(
                            HDA_VERB_GET_PARAMETER,
                            HDA_PARAM_PIN_CAPABILITIES,
                            0,
                            widget_i,
                            codec_i);
                        std::print("  capable of input: {}\n", (pin_capabilities & HDA_PIN_CAP_INPUT) != 0);
                        std::print("  capable of output: {}\n", (pin_capabilities & HDA_PIN_CAP_OUTPUT) != 0);

                        // Get list of possible inputs, trying to find a path to a DAC, possibly
                        // through some amount of mixer/selector widgets.
                        auto connection_count = send_command(
                            HDA_VERB_GET_PARAMETER,
                            HDA_PARAM_CONN_LIST_LENGTH,
                            0,
                            widget_i,
                            codec_i);
                        bool is_long_form = connection_count & 0x80;
                        connection_count &= 0x7f;
                        std::print("  {} connections\n", connection_count);

                        // Read the connection list elements using HDA_VERB_GET_CONN_LIST_ENTRY
                        uint dac_id = uint(-1);
                        for (uint32_t connection_group_i = 0; connection_group_i < connection_count;) {
                            auto connection_entries = send_command(
                                HDA_VERB_GET_CONN_LIST_ENTRY,
                                connection_group_i,
                                0,
                                widget_i,
                                codec_i);

                            if (is_long_form) {
                                // 16-bit Node IDs (2 entries per response payload)
                                uint16_t node0 = connection_entries & 0xffff;
                                uint16_t node1 = (connection_entries >> 16) & 0xffff;

                                ++connection_group_i;
                                std::print("    node: {}\n", node0);
                                if (widget_types[node0] != u8(-1))
                                    std::print("    type: {}\n", widget_types[node0]);

                                if (connection_group_i < connection_count) {
                                    ++connection_group_i;
                                    std::print("   node: {}\n", node1);
                                    if (widget_types[node1] != u8(-1))
                                        std::print("    type: {}\n", widget_types[node1]);
                                }
                            }
                            else {
                                // 8-bit Node IDs (4 entries per response payload)
                                for (int shift = 0; shift < 32; shift += 8) {
                                    if (connection_group_i >= connection_count) break;
                                    ++connection_group_i;

                                    uint8_t target_node = (connection_entries >> shift) & 0xFF;

                                    // Handle Node ID Ranges (e.g., node X through Y if bit 7 of the byte is set)
                                    // Note: Advanced range checking can be added here if needed
                                    std::print("    node: {}\n", target_node);
                                    if (widget_types[target_node] != u8(-1)) {
                                        std::print("    type: {}\n", widget_types[target_node]);
                                        // DAC
                                        if (widget_types[target_node] == HDA_WIDGET_TYPE_AUDIO_OUTPUT) {
                                            dac_id = target_node;
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        // TODO: If we aren't going to use this pin, we could power it down.
                        if ((pin_capabilities & HDA_PIN_CAP_OUTPUT) == 0)
                            continue;

                        if (dac_id == uint(-1)) {
                            std::print("  could not find DAC connected to pin complex\n");
                            continue;
                        }

                        if (connection_count > 1) {
                            // TODO: Use Set Connection Select (HDA_VERB_SET_CONN_SELECT) to select input path
                        }

                        // enable DAC
                        // 1. power on
                        std::print("  (dac): powering on\n");
                        send_command(HDA_VERB_SET_POWER_STATE, 0, 0, dac_id, codec_i);
                        // 2. amp gain/unmute
                        const uint dac_volume = 0x7f;  // 100% == 0x7f
                        std::print("  (dac): unmuting left, right channels for output. volume:{:#x}\n", dac_volume);
                        send_command(
                            HDA_VERB_SET_AMP_GAIN_MUTE,
                            HDA_AMP_PARAM_SET_LEFT | HDA_AMP_PARAM_SET_RIGHT
                                | HDA_AMP_PARAM_SET_OUTPUT,

                            dac_volume,
                            dac_id,
                            codec_i);
                        // 3. configure stream format
                        // NOTE: 16-bit payload split across param (high byte) and data (low byte)
                        // Format: Intel HDA 3.7.1 Stream Format Structure
                        // Bit 15: TYPE
                        //     0 -> PCM, 1 -> Non-PCM
                        //     This bit changes the meaning of all other bits.
                        //     The following is a description of the PCM format.
                        // Bit 14: BASE
                        //     0 -> 48kHz, 1 -> 44.1kHz
                        // Bit 13: RESERVED
                        // Bits [12:11]: MULT
                        //     00 -> BASE
                        //     01 -> x2
                        //     10 -> x3
                        //     11 -> x4
                        // Bits [10:8]: DIV
                        //     000 -> By 1
                        //     001 -> By 2
                        //     010 -> By 3
                        //     011 -> By 4
                        //     100 -> By 5
                        //     101 -> By 6
                        //     110 -> By 7
                        //     111 -> By 8
                        // Bit 7: RESERVED
                        // Bits [6:4]: BITS
                        //     Specifies number of bits in each sample.
                        //     000 ->  8 bits. The data is packed in memory in  8-bit containers on 16-bit boundaries.
                        //     001 -> 16 bits. The data is packed in memory in 16-bit containers on 16-bit boundaries.
                        //     010 -> 20 bits. The data is packed in memory in 20-bit containers on 32-bit boundaries.
                        //     011 -> 24 bits. The data is packed in memory in 24-bit containers on 32-bit boundaries.
                        //     100 -> 32 bits. The data is packed in memory in 32-bit containers on 32-bit boundaries.
                        //     101-111 -> RESERVED
                        // Bit 3:0 CHAN:
                        //     Specifies the number of channels for this stream in each "sample block"
                        //     of the "packets" in each "frame" on the link.
                        //
                        //     0000 -> 1
                        //     0001 -> 2
                        //     ...
                        //     1111 -> 16
                        //
                        //     Effectively, this value is the number of channels minus one, max of 16.
                        uint16_t format_data{
                            (uint16_t(0b0) << 15)     // PCM
                            | (uint16_t(0b0) << 14)   // 48kHz
                            | (uint16_t(0b00) << 11)  // Multiply by 1
                            | (uint16_t(0b000) << 8)  // Divide by 1
                            | (uint16_t(0b001) << 4)  // 16-bits
                            | uint16_t(0b0001)        // 2 channels (stereo)
                        };
                        send_command(
                            HDA_VERB_SET_CONV_FMT,
                            format_data >> 8,
                            format_data & 0xff,
                            dac_id,
                            codec_i);

                        // 4. configure stream descriptors
                        //   4a. first, actually allocate and configure the streams we want to
                        //       describe.
                        // An index of a valid output stream (i.e. not in use).
                        // TODO: Find one by iterating output streams
                        constexpr auto stream_index = 0;
                        // Any value 1 thru 15
                        constexpr auto stream_id = 1;
                        std::print(
                            "  utilizing output stream [{}] -> assigning id {}\n",
                            stream_index,
                            stream_id);
                        std::print(
                            "[HDA] input streams={} output streams={} bidi streams={}\n",
                            global_regs()->input_stream_count(),
                            global_regs()->output_stream_count(),
                            global_regs()->bidi_stream_count());
                        volatile auto* output_stream_registers
                            = (volatile IntelHDAStreamRegs*)(Base
                                                             + HDA_REG_OSD(
                                                                 global_regs()->input_stream_count(),
                                                                 stream_index,
                                                                 0));
                        std::print(
                            "[HDA]: status:{:#x} control:{:#x}\n",
                            output_stream_registers->status(),
                            output_stream_registers->control());
                        // Ensure the stream is stopped.
                        //   clear run bit
                        std::print("  stopping stream[{}]\n", stream_index);
                        output_stream_registers->control(
                            output_stream_registers->control() & ~HDA_SD_REG_CTL_RUN);
                        std::print(
                            "[HDA]: status:{:#x} control:{:#x}\n",
                            output_stream_registers->status(),
                            output_stream_registers->control());

                        // Write chosen Stream ID to identify active stream.
                        std::print("  assigning id {} to stream[{}]\n", stream_id, stream_index);
                        auto control_value = output_stream_registers->control();
                        control_value &= ~(0xf << 20);
                        std::print("control value: {:#x}\n", control_value);
                        // HDA_SD_REG_CTL_STREAM(stream_id) == 1048576 (0x100000)
                        control_value |= HDA_SD_REG_CTL_STREAM(stream_id);
                        std::print("control value w/ id: {:#x}\n", control_value);
                        std::print("expected control value w/o status: {:#x}\n", control_value << 8);
                        output_stream_registers->control(control_value);
                        std::print(
                            "[HDA]: status:{:#x} control:{:#x}\n",
                            output_stream_registers->status(),
                            output_stream_registers->control());

                        //   perform hardware reset
                        // std::print("  resetting stream[{}]\n", stream_index);
                        // output_stream_registers->stream_reset();
                        // std::print(
                        //     "[HDA]: status:{:#x} control:{:#x}\n",
                        //     output_stream_registers->status(),
                        //     output_stream_registers->control());

                        // Configure stream format.
                        std::print("  stream[{}] setting format: {:#x}\n", stream_index, format_data);
                        output_stream_registers->fmt = format_data;
                        std::print(
                            "[HDA]: status:{:#x} control:{:#x}\n",
                            output_stream_registers->status(),
                            output_stream_registers->control());

                        // Allocate buffers for BDL
                        constexpr auto buffer_count = 2u;
                        constexpr auto buffer_size = PAGE_SIZE * 2;
                        constexpr auto buffer_pages = buffer_size / PAGE_SIZE;
                        constexpr auto buffer_total_size = buffer_size * buffer_count;

                        BufferCount = buffer_count;

                        uint32_t total_samples_played = 0;

                        SYSTEM->audio_config.sample_buffer_byte_size = buffer_size;
                        SYSTEM->audio_config.sample_buffer_count = buffer_count;

                        auto* buffer_descriptors = (IntelHDABdlEntry*)Memory::request_page();
                        memset(buffer_descriptors, 0, PAGE_SIZE);
                        for (auto i = 0u; i < buffer_count; ++i) {
                            auto* buffer_base = Memory::request_pages(buffer_pages);
                            auto buffer_physical = Memory::TO_FRAME_POINTER(PhysicalKernelAddress(buffer_base));
                            std::print("  allocated audio sample buffer[{}] at {}, length={}\n", i, (void*)buffer_physical.address, buffer_size);
                            // silence!
                            memset(buffer_base, 0, buffer_size);
                            buffer_descriptors[i].length = buffer_size;
                            buffer_descriptors[i].address = buffer_physical.address;

                            // Initialize audio config for hardware query syscall
                            SYSTEM->audio_config.sample_buffers[i] = (void*)buffer_physical.address;

                            /** (!) TEST: SQUARE WAVE (!) **/
                            {
                                // 48000 Hz / 100 periods = 480 samples per full wave cycle (a clean 100 Hz bass tone)
                                // Changing the tone to alternate every 240 samples creates a perfect 100Hz square wave
                                constexpr auto half_period = 128u;

                                auto* buffer = (int16_t*)buffer_base;

                                for (auto i = 0u; i < buffer_size / sizeof(int16_t); i++) {
                                    int16_t sample_value;

                                    // Determine if we are in the positive or negative phase of the wave
                                    if ((total_samples_played / half_period) % 2 == 0) {
                                        sample_value = 16384;  // Half of the maximum 16-bit range (Safe headroom)
                                    }
                                    else {
                                        sample_value = -16384;  // Half of the minimum 16-bit range
                                    }

                                    // Fill interleaved Intel HDA stereo channels
                                    buffer[i * 2] = sample_value;      // Left
                                    buffer[i * 2 + 1] = sample_value;  // Right

                                    // Increment the global counter safely
                                    total_samples_played++;
                                }
                            }

                            std::print(
                                "  bdl[{}]: length={} address={} ioc={}\n",
                                i,
                                buffer_descriptors[i].length,
                                (void*)buffer_descriptors[i].address,
                                buffer_descriptors[i].ioc);
                        }

                        // Configure BDL
                        const auto buffer_descriptors_physical = Memory::TO_FRAME_POINTER(PhysicalKernelAddress(buffer_descriptors));
                        std::print("  buffer descriptor list pointer: {}\n", (void*)buffer_descriptors_physical.address);
                        std::print(
                            "[HDA]: status:{:#x} control:{:#x}\n",
                            output_stream_registers->status(),
                            output_stream_registers->control());
                        output_stream_registers->bdlp = buffer_descriptors_physical.address;
                        output_stream_registers->cbl = buffer_total_size;
                        output_stream_registers->lvi = buffer_count - 1;
                        std::print(
                            "[HDA]: bdlp={} cbl={} lvi={}\n",
                            (void*)output_stream_registers->bdlp,
                            (uintptr_t)output_stream_registers->cbl,
                            (uintptr_t)output_stream_registers->lvi);
                        std::print(
                            "[HDA]: status:{:#x} control:{:#x}\n",
                            output_stream_registers->status(),
                            output_stream_registers->control());
                        output_stream_registers->status_clear();

                        //   4b. then, tell the DAC to take input from that stream
                        // Stream is in bits 7:4
                        // Channel is in bits 3:0
                        send_command(
                            HDA_VERB_SET_CONV_STREAM_CHAN,
                            ((stream_id & 0xf) << 4),
                            0,
                            dac_id,
                            codec_i);

                        // Unmute pin widget
                        const uint pin_volume = 0x7f;  // 100% == 0x7f
                        std::print("  unmuting left, right channels for output. volume:{:#x}\n", pin_volume);
                        send_command(
                            HDA_VERB_SET_AMP_GAIN_MUTE,
                            HDA_AMP_PARAM_SET_LEFT | HDA_AMP_PARAM_SET_RIGHT
                                | HDA_AMP_PARAM_SET_OUTPUT,

                            pin_volume,
                            widget_i,
                            codec_i);

                        // Enable pin widget
                        std::print("  enabling pin complex for output\n");
                        uint32_t pin_ctl_payload = HDA_PIN_CTL_ENABLE_OUTPUT;
                        // If headphone, configure VRef En (bottom 2 bits) and set bit 7
                        if (pin_complex_config.default_device() == 2)
                            pin_ctl_payload |= HDA_PIN_CTL_ENABLE_HP | 5;
                        send_command(
                            HDA_VERB_SET_PIN_WIDGET_CTL,
                            pin_ctl_payload,
                            0,
                            widget_i,
                            codec_i);

                        std::print(
                            "[HDA]: status:{:#x} control:{:#x}\n",
                            output_stream_registers->status(),
                            output_stream_registers->control());

                        asm volatile("wbinvd" ::: "memory");

                        // Set output stream running
                        // Set the run bit in the control register
                        // NOTE: This should cause audio to start playing...
                        // Ensure status write-1-to-clear bits are clear
                        output_stream_registers->status_clear();
                        output_stream_registers->control(
                            output_stream_registers->control() | HDA_SD_REG_CTL_RUN);

                        for (int i = 8000000; i > 0; --i) {
                            asm volatile("pause" ::: "memory");
                            if (i % 20000 == 0) {
                                std::print(
                                    "[HDA]: status:{:#x} control:{:#x}\n",
                                    output_stream_registers->status(),
                                    output_stream_registers->control());
                            }
                        }

                    } break;
                    case HDA_WIDGET_TYPE_POWER_STATE: {
                        std::print("  power state\n");
                    } break;
                    case HDA_WIDGET_TYPE_VOLUME_KNOB: {
                        std::print("  volume knob\n");
                    } break;
                    case HDA_WIDGET_TYPE_BEEP_GENERATOR: {
                        std::print("  beep generator\n");
                    } break;
                    default:
                        break;
                }
            }
        }
    }

    return true;
}

void HDAController::handle_interrupt() {
    constexpr auto output_stream_index = 0u;

    auto global_stream_index = global_regs()->input_stream_count() + output_stream_index;
    auto output_stream_mask = (1u << global_stream_index);

    // If this interrupt targets the configured output stream...
    volatile uint32_t* intsts = (volatile uint32_t*)(Base + HDA_REG_INTSTS);
    if ((*intsts & output_stream_mask) != 0) {
        // And the interrupt is a buffer completion interrupt (BCIS) bit set.
        volatile auto* output_stream_registers
            = (volatile IntelHDAStreamRegs*)(Base
                                             + HDA_REG_OSD(
                                                 global_regs()->input_stream_count(),
                                                 output_stream_index,
                                                 0));
        if (output_stream_registers->status() & (1u << 2)) {
            // This was a buffer completion interrupt; we need to "swap buffers" (let
            // audio server know that the current buffer has been played from and is
            // ready to be written to)
            auto buffer_ready_for_cpu = CurrentBufferIndex;

            Event e{EventType::AUDIOBUFFER};
            auto* edata = (EventData_AudioBuffer*)(&e.Data[0]);
            edata->buffer_viable = buffer_ready_for_cpu;
            gEvents.notify(e);

            // Flip ping-pong tracking flag for the next cycle
            CurrentBufferIndex = (CurrentBufferIndex + 1) % BufferCount;
        }

        // Ensure more interrupts can happen
        output_stream_registers->status_clear();
    }
}
