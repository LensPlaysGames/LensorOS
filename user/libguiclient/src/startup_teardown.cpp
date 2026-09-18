#include <guiclient/gui.h>
#include <lensor/files.h>
#include <stdint.h>
#include <sys/syscalls.h>

gui_info_t gGUIINFO{};

gui_framebuffer_t* gui_get_framebuffer(uintptr_t handle) {
    if (not handle) return nullptr;
    auto* info = (gui_info_t*)handle;
    return &info->framebuffer;
}

uintptr_t gui_startup() {
    gui_info_t* info = &gGUIINFO;

    // Get graphical window from opening a connection to the !GUI socket
    info->client_file_descriptor = std::sys_socket(0, 0, 0);
    sockaddr addr{sockaddr::LENSOR16, "!GUI"};
    auto rc = std::sys_connect(
        info->client_file_descriptor,
        &addr,
        sizeof(addr));
    if (rc) {
        std::sys_close(info->client_file_descriptor);
        printf("[TERM]: Couldn't connect to GUI Server (address: %s)\n", addr.data);
        fflush(stdout);
        return 0;
    }
    unsigned char data[32];
    ssize_t bytes_read = std::sys_read(
        info->client_file_descriptor,
        data,
        24,
        0);
    uint64_t* data_it = (uint64_t*)data;
    uint64_t shared_memory_id = data_it[2];
    uintptr_t* shared_data = (uintptr_t*)syscall(
        SYS_shared_memory_acquire,
        shared_memory_id);

    auto& framebuffer = info->framebuffer;
    framebuffer.base_address = (uintptr_t)shared_data;
    framebuffer.buffer_size = *shared_data++;
    framebuffer.pixel_width = *shared_data++;
    framebuffer.pixel_height = *shared_data++;
    // TODO: get from window server
    framebuffer.pixels_per_line = framebuffer.pixel_width;
    framebuffer.pixel_byte_width = 4;

    // clear visual artifact from passing data in shared memory region used by
    // framebuffer.
    memset(shared_data, 0, (size_t)bytes_read);

    return (uintptr_t)&gGUIINFO;
}

void gui_teardown(uintptr_t handle) {
    if (not handle) return;
    gui_info_t* info = (gui_info_t*)handle;

    if (info->framebuffer.base_address)
        syscall(SYS_shared_memory_release, info->framebuffer.base_address);
    info->framebuffer.base_address = 0;

    if (info->client_file_descriptor != (uintptr_t)ProcFD::Invalid)
        std::sys_close(info->client_file_descriptor);
    info->client_file_descriptor = (uintptr_t)ProcFD::Invalid;
}
