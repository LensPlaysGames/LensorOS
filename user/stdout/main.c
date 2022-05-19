#include <string.h>
#include <sys/syscalls.h>
//#include <unistd.h>

int main() {
  while (1) {
    syscall(SYS_poke);
  }
  //const char *message = "Hello, friends :^)";
  //write(1, message, strlen(message));
  return 0;
}
