#include <sys/syscalls.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SOCK_ADDR_MAX_SIZE 16

typedef struct sockaddr {
  enum {
    UNBOUND,
    LENSOR16,
  } type;
  unsigned char data[SOCK_ADDR_MAX_SIZE];
} sockaddr;

int main(int argc, char **argv) {
  printf("client starting...\n");
  fflush(stdout);
  int sockFD = syscall(SYS_socket, 0, 0, 0);

  sockaddr addr;
  addr.type = LENSOR16;
  const char socket_path[] = "!Test";
  memset(addr.data, 0, SOCK_ADDR_MAX_SIZE);
  memcpy(addr.data, &socket_path, sizeof(socket_path) - 1);
  int rc = syscall(SYS_connect, sockFD, (uintptr_t)&addr, sizeof(sockaddr));
  printf("  connect returned %d\n", rc);
  fflush(stdout);
  if (rc) {
    close(sockFD);
    printf("Couldn't connect to address: %s\n", socket_path);
    return rc;
  }
  printf("Connected!\n");

  unsigned char data[512];
  int bytes_read = 0;
  printf("Reading...\n");
  fflush(stdout);
  bytes_read += read(sockFD, data, 16);

  printf("Read %d bytes from socket\n", bytes_read);
  uint64_t* data_it = (uint64_t*)data;
  uint64_t leading = *data_it++;
  uint64_t trailing = *data_it;

  printf("  got %u and %u\n", leading, trailing);
  write(sockFD, data, 16);

  close(sockFD);
  return 0;
}
