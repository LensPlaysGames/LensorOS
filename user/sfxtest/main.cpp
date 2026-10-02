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

/** SerDio -> LensorOS Audio Server
 **
 ** Provide IPC mechanism for creating and configuring Audio Nodes.
 ** Handle LensorOS kernel DMA handshake.
 **/

#include <lensor/ipc.h>
#include <lensor/kqueue_events.h>
#include <sys/syscalls.h>

#include <print>

struct client_header_t {
    /** Client should fill samples with data, then set this to true.
     ** Client may either wait for IPC message (kqueue wait), or simply poll
     ** the audio and fill it whenever it happens to be ready.
     **
     ** Server should not mix samples unless this is true.
     ** Server should set this to false when it has mixed the samples.
     **/
    volatile bool ready{false};
    volatile int16_t* samples() const volatile { return (int16_t*)(this + 1); };
};

int main(int argc, const char** argv) {
    // Get audio context from opening a connection to the !SFX socket
    auto client_file_descriptor = std::sys_socket(0, 0, 0);
    sockaddr addr{sockaddr::LENSOR16, "!SFX"};
    auto rc = std::sys_connect(
        client_file_descriptor,
        &addr,
        sizeof(addr));
    if (rc) {
        std::sys_close(client_file_descriptor);
        printf("[TERM]: Couldn't connect to GUI Server (address: %s)\n", addr.data);
        fflush(stdout);
        return 0;
    }
    unsigned char data[32];
    ssize_t bytes_read = std::sys_read(
        client_file_descriptor,
        data,
        sizeof(int) * 2,
        LENSOROS_SYSCALL_READ_FLAG_NONE);
    int* data_it = (int*)data;
    int magic = data_it[0];
    int shared_memory_id = data_it[1];
    // TODO: read buffer size
    constexpr size_t buffer_size = 8192 - sizeof(client_header_t);
    volatile client_header_t* shared_data = (volatile client_header_t*)syscall(
        SYS_shared_memory_acquire,
        shared_memory_id);

    /** (!) TEST: SQUARE WAVE (!) **/
    uintptr_t total_samples_played{};
    {
        // 48000 Hz / 100 periods = 480 samples per full wave cycle (a clean 100 Hz bass tone)
        // Changing the tone to alternate every 240 samples creates a perfect 100Hz square wave
        constexpr auto half_period = 128u;

        auto* buffer = shared_data->samples();

        for (auto i = 0u; i + 1 < buffer_size / sizeof(int16_t); i += 2) {
            int16_t sample_value;

            // Determine if we are in the positive or negative phase of the wave
            if ((total_samples_played / half_period) % 2 == 0) {
                sample_value = 16384;  // Half of the maximum 16-bit range (Safe headroom)
            }
            else {
                sample_value = -16384;  // Half of the minimum 16-bit range
            }

            // Fill interleaved Intel HDA stereo channels
            buffer[i] = sample_value;      // Left
            buffer[i + 1] = sample_value;  // Right

            ++total_samples_played;
        }

        std::print("Audio prepared\n");

        shared_data->ready = true;
        // TODO: sleep until the audio server wakes us up again (kqueue listen to
        // client file descriptor for write).
        // Fill with more (maybe even different) audio!
    }

    while (shared_data->ready) {
        std::sys_cooperative_yield();
    }

    std::print("Played sound!\n");

    return 0;
}
