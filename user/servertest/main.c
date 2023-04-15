#include <sys/syscalls.h>

#include <stdio.h>
#include <stdint.h>
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
  printf("server starting...\n");
  fflush(stdout);
  int sockFD = syscall(SYS_socket, 0, 0, 0);

  sockaddr addr;
  addr.type = LENSOR16;
  const char socket_path[] = "!Test";
  memset(addr.data, 0, SOCK_ADDR_MAX_SIZE);
  memcpy(addr.data, &socket_path, sizeof(socket_path) - 1);
  // bind (set our address)
  syscall(SYS_bind, sockFD, &addr, sizeof(sockaddr));
  // listen (mark self as server)
  syscall(SYS_listen, sockFD, 32);

  printf("Waiting for a connection to come in...\n");
  sockaddr connected_addr;
  size_t connected_addrlen = sizeof(sockaddr);
  int clientFD = -1;
  do {
    printf("Accepting...\n");
    fflush(stdout);
    // We will block here until a connection is made.
    clientFD = syscall(SYS_accept, sockFD, &connected_addr, &connected_addrlen);
    printf("  accept returned %d\n", clientFD);
    fflush(stdout);
  }
  while (clientFD == -2)
    ;
  if (clientFD < 0) {
    close(sockFD);
    printf("`accept` failed: %d\n", clientFD);
    return 1;
  }
  printf("Connection accepted: clientFD=%d\n", clientFD);

  uint64_t payload[2] = {69, 420};

  printf("Server writing...\n");
  fflush(stdout);

  write(clientFD, payload, sizeof(payload));

  unsigned char data[512];
  int bytes_read = 0;

  printf("Server reading...\n");
  fflush(stdout);
  bytes_read += read(clientFD, data, 512);

  printf("Server read %d bytes from socket\n", bytes_read);
  fflush(stdout);

  close(clientFD);

  printf("Server shutting down\n");
  close(sockFD);

  return 0;
}
