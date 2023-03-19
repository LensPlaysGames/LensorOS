#include <stddef.h>
#include <stdio.h>

int main(int argc, const char **argv) {
  int i = 1;
  while (i < argc) {
    printf("%s", argv[i]);
    if (i + 1 < argc) putchar(' ');
    ++i;
  }
  putchar('\n');
}
