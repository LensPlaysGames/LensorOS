#include <sys/syscalls.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define SOCK_ADDR_MAX_SIZE 16

int main(int argc, char **argv) {
  printf("server starting...\n");
  fflush(stdout);
  int sockFD = sys_socket(0, 0, 0);

  sockaddr addr;
  addr.type = LENSOR16;
  const char socket_path[] = "!Test";
  memset(addr.data, 0, SOCK_ADDR_MAX_SIZE);
  memcpy(addr.data, &socket_path, sizeof(socket_path) - 1);
  // bind (set our address)
  sys_bind(sockFD, &addr, sizeof(sockaddr));
  // listen (mark self as server)
  sys_listen(sockFD, 32);

  // Open an event queue to be notified when an incoming connection is
  // coming in on the server socket. This isn't really needed; we could
  // use the blocking mechanism of accept() for this. However, with this
  // technique, it would theoretically be possible to be doing other
  // things first (like handling all current connections with another
  // event queue; we'll get there) before checking if any incoming
  // connections have come in.
  int listen_queue = sys_kqueue();

  const size_t changelist_size = 4;
  Event changelist[changelist_size];
  memset(changelist, 0, sizeof(changelist));
  changelist[0].Type = EVENTTYPE_READY_TO_READ;
  changelist[0].Filter.ProcessFD = sockFD;
  sys_kevent(listen_queue, changelist, 1, NULL, 0);

  printf("Waiting for a READY_TO_READ event to come in on the listen_queue...\n");
  fflush(stdout);
  const size_t eventlist_size = 4;
  Event eventlist[eventlist_size];
  memset(eventlist, 0, sizeof(eventlist));
  int status = 0;
  while ((status = sys_kevent(listen_queue, NULL, 0, eventlist, eventlist_size)) != 0)
    ;

  printf("Waiting for a connection to come in...\n");
  sockaddr connected_addr;
  size_t connected_addrlen = sizeof(sockaddr);
  int clientFD = -1;
  do {
    printf("Accepting...\n");
    fflush(stdout);
    // We will block here until a connection is made.
    clientFD = sys_accept(sockFD, &connected_addr, &connected_addrlen);
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
