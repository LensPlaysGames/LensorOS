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
    volatile int16_t* samples() const { return (int16_t*)(this + 1); };
};

struct Client {
    // The shared memory region between server and client.
    client_header_t* shared_region{nullptr};
    // The file descriptor the server uses to talk to the client.
    ProcFD client_fd;
    // How many samples fit in the process' region.
    uintptr_t sample_count{0};
};

static std::vector<int32_t> mix_buffer(4096, '\0');
void mix_audio(int16_t* dma_buffer, const std::vector<Client>& clients, size_t sample_count) {
    // Clear the mixing buffer.
    memset(
        mix_buffer.data(),
        0,
        mix_buffer.size() * sizeof(*mix_buffer.data()));

    // Accumulate all client samples into the mixing buffer.
    for (const auto& client : clients) {
        if ((not client.shared_region) or (not client.shared_region->ready))
            continue;

        const volatile int16_t* client_samples = client.shared_region->samples();

        for (size_t i = 0; i < sample_count; ++i)
            mix_buffer[i] += client_samples[i];

        client.shared_region->ready = false;
    }

    // Clamp to 16-bit boundaries and write to output DMA buffer.
    for (size_t i = 0; i < sample_count; ++i) {
        int32_t accumulated = mix_buffer[i];
        accumulated = std::clamp(accumulated, -32768, 32767);
        dma_buffer[i] = static_cast<int16_t>(accumulated);
    }
}

int main(int argc, const char** argv) {
    auto kqueue = std::sys_kqueue();
    {
        Event change{};
        change.Type = EventType::AUDIOBUFFER;
        std::sys_kevent(kqueue, &change, 1, nullptr, 0);
    }

    hardware_audio_t hardware{};
    auto rc = std::sys_audio_hardware(&hardware);
    if (rc != 0) {
        std::print("[SerDio]: Could not fetch audio hardware buffers\n");
        return -1;
    }
    const size_t sample_buffer_sample_count = hardware.sample_buffer_byte_size / sizeof(int16_t);

    for (auto i = 0u; i < hardware.sample_buffer_count; ++i) {
        std::print("[SerDio]: Buffer[{}]: {}\n", i, hardware.sample_buffers[i]);
        memset(hardware.sample_buffers[i], 0, hardware.sample_buffer_byte_size);
    }

    // Open audio socket for listening
    auto sockFD = std::sys_socket(0, 0, 0);
    sockaddr addr;
    addr.type = sockaddr::LENSOR16;
    const char socket_path[] = "!SFX";
    memset(addr.data, 0, SOCK_ADDR_MAX_SIZE);
    memcpy(addr.data, &socket_path, sizeof(socket_path) - 1);
    // bind (set our address)
    std::sys_bind(sockFD, &addr, sizeof(sockaddr));
    // listen (mark self as server)
    std::sys_listen(sockFD, 32);

    // register to listen for incoming connections
    {
        Event change{};
        change.Type = EventType::READY_TO_READ;
        change.Filter.ProcessFD = sockFD;
        change.Flags |= EVENTFLAGS_CHANGE_ADD_REMOVE;
        std::sys_kevent(kqueue, &change, 1, nullptr, 0);
    }

    Event event{};
    size_t active_sample_buffer_index{0};
    std::vector<Client> clients{};
    bool running = true;
    while (running) {
        auto rc = std::sys_kevent(kqueue, nullptr, 0, &event, 1);
        if (rc >= 0) {
            if (event.Type == EventType::AUDIOBUFFER) {
                auto* active_sample_buffer = (int16_t*)hardware.sample_buffers[active_sample_buffer_index];
                mix_audio(
                    active_sample_buffer,
                    clients,
                    sample_buffer_sample_count);

                active_sample_buffer_index = (active_sample_buffer_index + 1)
                                             % hardware.sample_buffer_count;
            }
            else if (
                event.Type == EventType::READY_TO_READ
                and event.Filter.ProcessFD == sockFD) {
                std::print("[SerDio]: Incoming Client\n");
                // TODO: accept new client connection
                auto new_client = Client();
                // TODO: allocate shared memory region(s); send client shared memory
                // region ID(s) over socket.
                clients.emplace_back(new_client);
            }
        }
        std::sys_cooperative_yield();
    }
}
