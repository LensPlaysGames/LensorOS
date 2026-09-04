#include <stdio.h>
#include <string.h>
#include <sys/syscalls.h>
#include <unistd.h>

int main(int argc, char** argv) {
    printf("[CLIENT]: starting... opening socket\n");
    fflush(stdout);

    int sockFD = sys_socket(0, 0, 0);

    printf("[CLIENT]: socket opened\n");
    fflush(stdout);

    sockaddr addr;
    addr.type = LENSOR16;
    const char socket_path[] = "!Test";
    memset(addr.data, 0, SOCK_ADDR_MAX_SIZE);
    memcpy(addr.data, &socket_path, sizeof(socket_path) - 1);
    int rc = sys_connect(sockFD, &addr, sizeof(sockaddr));
    if (rc) {
        close(sockFD);
        printf("[CLIENT]: Couldn't connect to address: %s\n", socket_path);
        fflush(stdout);
        return rc;
    }
    printf("[CLIENT]: Connected to address: \"%s\"\n", socket_path);
    fflush(stdout);

    printf("[CLIENT]: Reading from socket...\n");
    fflush(stdout);

    unsigned char data[512];
    int bytes_read = 0;
    bytes_read += read(sockFD, data, 24);

    printf("[CLIENT]: Read %d bytes from socket\n", bytes_read);
    fflush(stdout);

    uint64_t* data_it = (uint64_t*)data;
    uint64_t leading = *data_it++;
    uint64_t trailing = *data_it++;
    uint64_t shared_memory_id = *data_it++;

    printf("[CLIENT]:  got %u and %u\n", leading, trailing);
    printf("[CLIENT]:  shared memory id %u\n", shared_memory_id);
    fflush(stdout);

    uintptr_t* shared_data = (uintptr_t*)syscall(SYS_shared_memory_acquire, shared_memory_id);
    printf("[CLIENT]: shared_data %p\n", shared_data);

    *shared_data = 69;

    write(sockFD, data, 16);

    printf("[CLIENT]:  client written\n");
    fflush(stdout);

    printf("[CLIENT]:  closing client socket\n");
    fflush(stdout);

    close(sockFD);

    printf("[CLIENT]:  closed client socket\n");
    fflush(stdout);

    return 0;
}
