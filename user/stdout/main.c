#include <string.h>
#include <unistd.h>

int main() {
  const char *message = "Hello, friends :^)\r\n";
  write(0, message, strlen(message));
  return 0;
}
