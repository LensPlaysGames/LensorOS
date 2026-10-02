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

struct Client {
    // The shared memory region between server and client.
    volatile client_header_t* shared_region{nullptr};
    int shared_region_id{0};
    // The file descriptor the server uses to talk to the client.
    ProcFD client_fd{};
    // How many samples fit in the process' region.
    uintptr_t sample_count{0};
};

static std::vector<int32_t> mix_buffer(4096, '\0');
void mix_audio(int16_t* dma_buffer, const std::vector<Client>& clients, size_t dma_sample_count) {
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
        for (size_t i = 0; i < client.sample_count; ++i)
            mix_buffer[i] += client_samples[i];

        client.shared_region->ready = false;
    }

    // Clamp to 16-bit boundaries and write to output DMA buffer.
    for (size_t i = 0; i < std::min(dma_sample_count, mix_buffer.size()); ++i) {
        int32_t accumulated = mix_buffer[i];
        accumulated = std::clamp(accumulated, -32768, 32767);
        dma_buffer[i] = static_cast<int16_t>(accumulated);
    }
}

struct server_context_t {
    int kqueue{-1};
    ProcFD server_socket{};
    std::vector<Client> clients{};
};

void handle_client_disconnect(server_context_t& context, Client& client, size_t index) {
    // Unregister kqueue listening for client file descriptor. This ensures we
    // won't continue listening to a dead event source.
    Event change{};
    change.Type = EventType::READY_TO_READ;
    change.Filter.ProcessFD = client.client_fd;
    std::sys_kevent(context.kqueue, &change, 1, NULL, 0);

    // Close client file descriptor; we won't be talking to them much, now
    // that they are gone, anyway.
    std::sys_close(client.client_fd);

    // Free (and unmap) memory shared between us and the client.
    syscall(SYS_shared_memory_release, client.shared_region_id);

    // Actually remove the client from the global client list, invalidating
    // our client reference.
    context.clients.erase(context.clients.begin() + index);
}

void handle_incoming_client(server_context_t& context) {
    std::print("[SerDio]: Incoming Client\n");

    ProcFD clientFD{};
    sockaddr connected_addr;
    size_t connected_addrlen = sizeof(sockaddr);
    clientFD = std::sys_accept(
        context.server_socket,
        &connected_addr,
        &connected_addrlen);
    if (clientFD == ProcFD::Invalid) {
        std::print(
            "[SerDio]: `accept` failed: {}\n"
            "  -> incoming client ignored :(\n",
            (uintptr_t)clientFD);
        return;
    }

    // Listen to client IPC socket for incoming messages.
    Event change{};
    change.Type = EventType::READY_TO_READ;
    change.Filter.ProcessFD = clientFD;
    change.Flags = EVENTFLAGS_CHANGE_ADD_REMOVE;
    std::sys_kevent(context.kqueue, &change, 1, nullptr, 0);

    // FIXME: size matching DMA buffer
    constexpr size_t buffer_size = 8192;
    constexpr size_t sample_buffer_size = 8192 - sizeof(client_header_t);
    constexpr size_t sample_buffer_sample_count = sample_buffer_size / sizeof(int16_t);

    uintptr_t* shared_data = nullptr;
    int shmem_id = syscall(SYS_shared_memory_allocate, &shared_data, buffer_size);
    memset(shared_data, 0, buffer_size);

    // TODO: Write magic for new context init + new context shared memory id
    int init[2]{0x69, shmem_id};
    std::sys_write(
        clientFD,
        (uint8_t*)&init[0],
        sizeof(init),
        LENSOROS_SYSCALL_WRITE_FLAG_NONE);

    auto new_client = Client();
    new_client.client_fd = clientFD;
    new_client.shared_region_id = shmem_id;
    new_client.shared_region = (volatile client_header_t*)shared_data;
    new_client.sample_count = sample_buffer_sample_count;
    context.clients.emplace_back(new_client);
}

void handle_client_message(server_context_t& context, const Event& event) {
    EventData_ReadyToReadWrite* e_data = (EventData_ReadyToReadWrite*)&event.Data[0];

    // Find client that this event originates from.
    size_t index = 0;
    Client* client = NULL;
    for (auto& c : context.clients) {
        if (c.client_fd == event.Filter.ProcessFD) {
            client = &c;
            break;
        }
        ++index;
    }
    // Event tied to client that no longer exists
    if (client == NULL) {
        std::print("[SerDio]: Received message for dead client, ignoring\n");
        return;
    }

    // Event signals client disconnected.
    if (event.Flags & EVENTFLAGS_FILEREADY_EOF) {
        std::print("[SerDio]: Client closed\n");
        // Ensure client's window is closed; they have closed their IPC
        // communication socket.
        handle_client_disconnect(context, *client, index);
    }
    // Event signals client wrote a message.
    else if (event.Flags & EVENTFLAGS_FILEREADY_READ) {
        std::print("[SerDio]: Received client message\n");
        // TODO: Read IPC message(s) and perform relevant action(s)
        uint8_t ipc_buffer[IPC_MAX_SIZE];
        intptr_t ipc_bytes_read = std::sys_read(
            event.Filter.ProcessFD,
            &ipc_buffer[0],
            e_data->BytesAvailable,
            LENSOROS_SYSCALL_READ_FLAG_NONE);

        std::print("  msg: ");
        for (int i = 0; i < ipc_bytes_read; ++i)
            std::print("{}", ipc_buffer[i]);
        std::print("\n");

        uint8_t magic = ipc_buffer[0];
    }
}

int main(int argc, const char** argv) {
    server_context_t context{};
    context.kqueue = std::sys_kqueue();
    {
        Event change{};
        change.Type = EventType::AUDIOBUFFER;
        change.Flags = EVENTFLAGS_CHANGE_ADD_REMOVE;
        std::sys_kevent(context.kqueue, &change, 1, nullptr, 0);
    }

    hardware_audio_t hardware{};
    auto rc = std::sys_audio_hardware(&hardware);
    if (rc != 0) {
        std::print("[SerDio]: Could not fetch audio hardware buffers\n");
        return -1;
    }
    const size_t hardware_sample_buffer_sample_count = hardware.sample_buffer_byte_size / sizeof(int16_t);

    for (auto i = 0u; i < hardware.sample_buffer_count; ++i) {
        std::print("[SerDio]: Buffer[{}]: {}\n", i, hardware.sample_buffers[i]);
        memset(hardware.sample_buffers[i], 0, hardware.sample_buffer_byte_size);
    }

    // Open audio socket for listening
    context.server_socket = std::sys_socket(0, 0, 0);
    sockaddr addr;
    addr.type = sockaddr::LENSOR16;
    const char socket_path[] = "!SFX";
    memset(addr.data, 0, SOCK_ADDR_MAX_SIZE);
    memcpy(addr.data, &socket_path, sizeof(socket_path) - 1);
    // bind (set our address)
    std::sys_bind(context.server_socket, &addr, sizeof(sockaddr));
    // listen (mark self as server)
    std::sys_listen(context.server_socket, 32);

    // register to listen for incoming connections
    {
        Event change{};
        change.Type = EventType::READY_TO_READ;
        change.Filter.ProcessFD = context.server_socket;
        change.Flags |= EVENTFLAGS_CHANGE_ADD_REMOVE;
        std::sys_kevent(context.kqueue, &change, 1, nullptr, 0);
    }

    Event event{};
    size_t active_sample_buffer_index{0};
    bool running = true;
    while (running) {
        auto rc = std::sys_kevent(context.kqueue, nullptr, 0, &event, 1);
        if (rc >= 0) {
            if (event.Type == EventType::AUDIOBUFFER) {
                auto* active_sample_buffer = (int16_t*)hardware.sample_buffers[active_sample_buffer_index];
                mix_audio(
                    active_sample_buffer,
                    context.clients,
                    hardware_sample_buffer_sample_count);

                active_sample_buffer_index = (active_sample_buffer_index + 1)
                                             % hardware.sample_buffer_count;
            }
            else if (
                event.Type == EventType::READY_TO_READ) {
                if (event.Filter.ProcessFD == context.server_socket)
                    handle_incoming_client(context);
                else
                    handle_client_message(context, event);
            }
        }
        std::sys_cooperative_yield();
    }
}
