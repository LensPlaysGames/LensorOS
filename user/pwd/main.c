#include <stdbool.h>
#include <stdio.h>
#include <sys/syscalls.h>

#define PWD_MAX 4096
static char pwdbuf[PWD_MAX];

int main(void) {
  pwdbuf[0] = '\0';
  if (!syscall(SYS_pwd, pwdbuf, PWD_MAX)) return 1;
  printf("%s", pwdbuf);
  return 0;
}
