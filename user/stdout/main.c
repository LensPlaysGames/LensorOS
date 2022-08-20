#include <string.h>
#include <unistd.h>

int main() {
  const char *message = "Hello, friends :^)\r\n";
  write(STDOUT_FILENO, message, strlen(message));
  return 0;
}
