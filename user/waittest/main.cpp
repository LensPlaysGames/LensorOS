#include <sys/syscalls.h>

#include <print>

int main(int argc, const char** argv) {
    std::sys_wait_milliseconds(1000);
    std::print("Waited!\n");
    return 0;
}
