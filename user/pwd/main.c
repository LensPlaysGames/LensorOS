#include <stdbool.h>
#include <stdio.h>
#include <sys/syscalls.h>

#define PWD_MAX 4096
static char pwdbuf[PWD_MAX];

int main(void) {
  // TODO: Handle -L and -P arguments.
  pwdbuf[0] = '\0';
  if (!syscall(SYS_pwd, pwdbuf, PWD_MAX)) return 1;
  printf("%s\n", pwdbuf);
  return 0;
}
