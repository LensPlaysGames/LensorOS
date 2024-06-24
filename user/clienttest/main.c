#include <sys/syscalls.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
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
  bytes_read += read(sockFD, data, 16);

  printf("[CLIENT]: Read %d bytes from socket\n", bytes_read);
  fflush(stdout);

  uint64_t* data_it = (uint64_t*)data;
  uint64_t leading = *data_it++;
  uint64_t trailing = *data_it;

  printf("[CLIENT]:  got %u and %u\n", leading, trailing);
  fflush(stdout);

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
